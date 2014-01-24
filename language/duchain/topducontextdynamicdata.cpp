/* This  is part of KDevelop

   Copyright 2014 Milian Wolff <mail@milianw.de>
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "topducontextdynamicdata.h"

#include <kstandarddirs.h>
#include <typeinfo>
#include <QtCore/QFile>
#include <QtCore/QByteArray>

#include "declaration.h"
#include "declarationdata.h"
#include "ducontext.h"
#include "topducontext.h"
#include "topducontextdata.h"
#include "ducontextdata.h"
#include "ducontextdynamicdata.h"
#include "duchainregister.h"
#include "repositories/itemrepository.h"

//#define DEBUG_DATA_INFO

//This might be problematic on some systems, because really many mmaps are created
#define USE_MMAP
using namespace KDevelop;

static QMutex s_temporaryDataMutex(QMutex::Recursive);

namespace {

void saveDUChainItem(QList<ArrayWithPosition>& data, DUChainBase& item, uint& totalDataOffset) {

  if(!item.d_func()->classId) {
    //If this triggers, you have probably created an own DUChainBase based class, but haven't called setClassId(this) in the constructor.
    kError() << QString("no class-id set for data attached to a declaration of type %1").arg(typeid(item).name());
    Q_ASSERT(0);
  }

  int size = DUChainItemSystem::self().dynamicSize(*item.d_func());

  if(data.back().first.size() - int(data.back().second) < size)
      //Create a new data item
      data.append( qMakePair(QByteArray(size > 10000 ? size : 10000, 0), 0u) );

  uint pos = data.back().second;
  data.back().second += size;
  totalDataOffset += size;

  DUChainBaseData& target(*(reinterpret_cast<DUChainBaseData*>(data.back().first.data() + pos)));

  if(item.d_func()->isDynamic()) {
    //Change from dynamic data to constant data

    enableDUChainReferenceCounting(data.back().first.data(), data.back().first.size());
    DUChainItemSystem::self().copy(*item.d_func(), target, true);
    Q_ASSERT(!target.isDynamic());
    item.setData(&target);
    disableDUChainReferenceCounting(data.back().first.data());
  }else{
    //Just copy the data into another place, expensive copy constructors are not needed
    memcpy(&target, item.d_func(), size);
    item.setData(&target, false);
  }
  Q_ASSERT(item.d_func() == &target);

  Q_ASSERT(!item.d_func()->isDynamic());
}

uint indexForParentContext(DUContext* context)
{
  return LocalIndexedDUContext(context->parentContext()).localIndex();
}

uint indexForParentContext(Declaration* declaration)
{
  return LocalIndexedDUContext(declaration->context()).localIndex();
}

void validateItem(const DUChainBase* const item, const uchar* const mappedData, const size_t mappedDataSize)
{
  Q_ASSERT(!item->d_func()->isDynamic());
  if (mappedData) {
    Q_ASSERT(((size_t)item->d_func()) < ((size_t)mappedData)
          || ((size_t)item->d_func()) > ((size_t)mappedData) + mappedDataSize);
  }
}

const char* pointerInData(const QList<ArrayWithPosition>& data, uint totalOffset)
{

  for(int a = 0; a < data.size(); ++a) {
    if(totalOffset < data[a].second)
      return data[a].first.constData() + totalOffset;
    totalOffset -= data[a].second;
  }
  Q_ASSERT_X(false, Q_FUNC_INFO, "Offset doesn't exist in the data.");
  return 0;
}

void verifyDataInfo(const TopDUContextDynamicData::ItemDataInfo& info, const QList<ArrayWithPosition>& data)
{
  Q_UNUSED(info);
  Q_UNUSED(data);
#ifdef DEBUG_DATA_INFO
  DUChainBaseData* item = (DUChainBaseData*)(pointerInData(data, info.dataOffset));
  int size = DUChainItemSystem::self().dynamicSize(*item);
  Q_ASSERT(size);
#endif
}

}

//BEGIN DUChainItemStorage

template<class Item>
TopDUContextDynamicData::DUChainItemStorage<Item>::DUChainItemStorage(TopDUContextDynamicData* data)
  : data(data)
{
}

template<class Item>
TopDUContextDynamicData::DUChainItemStorage<Item>::~DUChainItemStorage()
{
  clearItems();
}

template<class Item>
void TopDUContextDynamicData::DUChainItemStorage<Item>::clearItems()
{
  //Due to template specialization it's possible that a declaration is not reachable through the normal context structure.
  //For that reason we have to check here, and delete all remaining declarations.
  qDeleteAll(temporaryItems);
  qDeleteAll(items);
  //NOTE: not clearing, is called oly from the dtor anyways
}

template<class Item>
void TopDUContextDynamicData::DUChainItemStorage<Item>::clearItemIndex(Item* item, const uint index)
{
  if(!data->m_dataLoaded)
    data->loadData();

  if (index < (0x0fffffff/2)) {
    if (index == 0 || index > uint(items.size())) {
      return;
    } else {
      const uint realIndex = index - 1;
      Q_ASSERT(items[realIndex] == item);
      items[realIndex] = nullptr;

      if (realIndex < (uint)offsets.size()) {
        offsets[realIndex] = ItemDataInfo();
      }
    }
  } else {
    QMutexLocker lock(&s_temporaryDataMutex);
    const uint realIndex = 0x0fffffff - index; //We always keep the highest bit at zero
    if (realIndex == 0 || realIndex > uint(temporaryItems.size())) {
      return;
    } else {
      Q_ASSERT(temporaryItems[realIndex-1] == item);
      temporaryItems[realIndex-1] = nullptr;
    }
  }
}

template<class Item>
void TopDUContextDynamicData::DUChainItemStorage<Item>::storeData(uint& currentDataOffset, const QList<ArrayWithPosition>& oldData)
{
  auto const oldOffsets = offsets;
  offsets.clear();
  for (int a = 0; a < items.size(); ++a) {
    auto item = items[a];
    if (!item) {
      if (oldOffsets.size() > a && oldOffsets[a].dataOffset) {
        //Directly copy the old data range into the new data
        const DUChainBaseData* itemData = nullptr;
        if (data->m_mappedData) {
          itemData = reinterpret_cast<const DUChainBaseData*>(data->m_mappedData + oldOffsets[a].dataOffset);
        } else {
          itemData = reinterpret_cast<const DUChainBaseData*>(::pointerInData(oldData, oldOffsets[a].dataOffset));
        }
        offsets << data->writeDataInfo(oldOffsets[a], itemData, currentDataOffset);
      } else {
        offsets << ItemDataInfo();
      }
    } else {
      offsets << ItemDataInfo(currentDataOffset, indexForParentContext(item));
      saveDUChainItem(data->m_data, *item, currentDataOffset);
      verifyDataInfo(offsets.back(), data->m_data);

      validateItem(item, data->m_mappedData, data->m_mappedDataSize);
    }
  }

#ifndef QT_NO_DEBUG
  for (auto item : items) {
    if (item) {
      validateItem(item, data->m_mappedData, data->m_mappedDataSize);
    }
  }
#endif
}

template<typename Item>
bool TopDUContextDynamicData::DUChainItemStorage<Item>::itemsHaveChanged() const
{
  for (auto item : items) {
    if (item && item->d_ptr->m_dynamic) {
      return true;
    }
  }
  return false;
}

template<class Item>
uint TopDUContextDynamicData::DUChainItemStorage<Item>::allocateItemIndex(Item* item, const bool temporary)
{
  if (!data->m_dataLoaded) {
    data->loadData();
  }
  if (!temporary) {
    items.append(item);
    return items.size();
  } else {
    QMutexLocker lock(&s_temporaryDataMutex);
    temporaryItems.append(item);
    return 0x0fffffff - temporaryItems.size(); //We always keep the highest bit at zero
  }
}

template<class Item>
bool TopDUContextDynamicData::DUChainItemStorage<Item>::isItemForIndexLoaded(uint index) const
{
  if (index < (0x0fffffff/2)) {
    if (index == 0 || index > uint(items.size())) {
      return false;
    }
    return items[index-1];
  } else {
    // temporary item
    return true;
  }
}

template<class Item>
Item* TopDUContextDynamicData::DUChainItemStorage<Item>::getItemForIndex(uint index) const
{
  if (index >= (0x0fffffff/2)) {
    QMutexLocker lock(&s_temporaryDataMutex);
    index = 0x0fffffff - index; //We always keep the highest bit at zero
    if(index == 0 || index > uint(temporaryItems.size()))
      return 0;
    else
      return temporaryItems[index-1];
  }

  if (index == 0 || index > static_cast<uint>(items.size())) {
    kWarning() << "item index out of bounds:" << index << "count:" << items.size();
    return nullptr;
  }
  const uint realIndex = index - 1;
  Item*& item = items[realIndex];
  if (item) {
    //Shortcut, because this is the most common case
    return item;
  }

  if (realIndex < (uint)offsets.size() && offsets[realIndex].dataOffset) {
    Q_ASSERT(!data->m_itemRetrievalForbidden);

    //Construct the context, and eventuall its parent first
    ///TODO: ugly, remove need for const_cast
    auto itemData = const_cast<DUChainBaseData*>(
      reinterpret_cast<const DUChainBaseData*>(data->pointerInData(offsets[realIndex].dataOffset))
    );

    item = dynamic_cast<Item*>(DUChainItemSystem::self().create(itemData));
    if (!item) {
      //When this happens, the item has not been registered correctly.
      //We can stop here, because else we will get crashes later.
      kError() << "Failed to load item with identity" << itemData->classId;
    }

    auto parent = data->getContextForIndex(offsets[realIndex].parentContext);
    Q_ASSERT_X(parent, Q_FUNC_INFO, "Could not find parent context for loaded item.\n"
                                    "Potentially, the context has been deleted without deleting its children.");
    item->rebuildDynamicData(parent, index);
  }

  return item;
}

template<class Item>
void TopDUContextDynamicData::DUChainItemStorage<Item>::deleteOnDisk()
{
  for (Item* item : items) {
    if (item) {
      item->makeDynamic();
    }
  }
}

template<class Item>
void TopDUContextDynamicData::DUChainItemStorage<Item>::loadData(QFile* file) const
{
  Q_ASSERT(offsets.isEmpty());
  Q_ASSERT(items.isEmpty());

  uint readValue;
  file->read((char*)&readValue, sizeof(uint));
  offsets.resize(readValue);

  file->read((char*)offsets.data(), sizeof(ItemDataInfo) * offsets.size());

  //Fill with zeroes for now, will be initialized on-demand
  items.resize(offsets.size());
}

//END DUChainItemStorage

const char* KDevelop::TopDUContextDynamicData::pointerInData(uint totalOffset) const {
  Q_ASSERT(!m_mappedData || m_data.isEmpty());

  if(m_mappedData && m_mappedDataSize)
    return (char*)m_mappedData + totalOffset;

  for(int a = 0; a < m_data.size(); ++a) {
    if(totalOffset < m_data[a].second)
      return m_data[a].first.constData() + totalOffset;
    totalOffset -= m_data[a].second;
  }
  Q_ASSERT(0); //Offset doesn't exist in the data
  return 0;
}

TopDUContextDynamicData::TopDUContextDynamicData(TopDUContext* topContext)
  : m_deleting(false)
  , m_topContext(topContext)
  , m_contexts(this)
  , m_declarations(this)
  , m_onDisk(false)
  , m_dataLoaded(true)
  , m_mappedFile(0)
  , m_mappedData(0)
  , m_mappedDataSize(0)
  , m_itemRetrievalForbidden(false)
{
}

void KDevelop::TopDUContextDynamicData::clearContextsAndDeclarations()
{
  m_contexts.clearItems();
  m_declarations.clearItems();
}

TopDUContextDynamicData::~TopDUContextDynamicData()
{
  unmap();
}

void KDevelop::TopDUContextDynamicData::unmap() {
  delete m_mappedFile;
  m_mappedFile = 0;
  m_mappedData = 0;
  m_mappedDataSize = 0;
}

QList<IndexedDUContext> TopDUContextDynamicData::loadImporters(uint topContextIndex) {
  QList<IndexedDUContext> ret;

  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  QString fileName = baseDir + '/' + QString("%1").arg(topContextIndex);
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly)) {
     uint readValue;
     file.read((char*)&readValue, sizeof(uint));
     //now readValue is filled with the top-context data size

     //We only read the most needed stuff, not the whole top-context data
     QByteArray data = file.read(readValue);
     const TopDUContextData* topData = reinterpret_cast<const TopDUContextData*>(data.constData());
     FOREACH_FUNCTION(const IndexedDUContext& importer, topData->m_importers)
      ret << importer;
  }

  return ret;
}

QList<IndexedDUContext> TopDUContextDynamicData::loadImports(uint topContextIndex) {
  QList<IndexedDUContext> ret;

  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  QString fileName = baseDir + '/' + QString("%1").arg(topContextIndex);
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly)) {
     uint readValue;
     file.read((char*)&readValue, sizeof(uint));
     //now readValue is filled with the top-context data size

     //We only read the most needed stuff, not the whole top-context data
     QByteArray data = file.read(readValue);
     const TopDUContextData* topData = reinterpret_cast<const TopDUContextData*>(data.constData());
     FOREACH_FUNCTION(const DUContext::Import& import, topData->m_importedContexts)
      ret << import.indexedContext();
  }

  return ret;
}

bool TopDUContextDynamicData::fileExists(uint topContextIndex)
{
  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  QString fileName = baseDir + '/' + QString("%1").arg(topContextIndex);
  QFile file(fileName);
  return file.exists();
}

IndexedString TopDUContextDynamicData::loadUrl(uint topContextIndex) {

  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  QString fileName = baseDir + '/' + QString("%1").arg(topContextIndex);
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly)) {
     uint readValue;
     file.read((char*)&readValue, sizeof(uint));
     //now readValue is filled with the top-context data size

     //We only read the most needed stuff, not the whole top-context data
     QByteArray data = file.read(sizeof(TopDUContextData));
     const TopDUContextData* topData = reinterpret_cast<const TopDUContextData*>(data.constData());
     Q_ASSERT(topData->m_url.isEmpty() || topData->m_url.index() >> 16);
     return topData->m_url;
  }

  return IndexedString();
}

void TopDUContextDynamicData::loadData() const {
  //This function has to be protected by an additional mutex, since it can be triggered from multiple threads at the same time
  static QMutex mutex;
  QMutexLocker lock(&mutex);
  if(m_dataLoaded)
    return;

  Q_ASSERT(!m_dataLoaded);
  Q_ASSERT(m_data.isEmpty());

  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  KStandardDirs::makeDir(baseDir);

  QString fileName = baseDir + '/' + QString("%1").arg(m_topContext->ownIndex());
  QFile* file = new QFile(fileName);
  bool open = file->open(QIODevice::ReadOnly);
  Q_UNUSED(open);
  Q_ASSERT(open);
  Q_ASSERT(file->size());

  //Skip the offsets, we're already read them
  //Skip top-context data
  uint readValue;
  file->read((char*)&readValue, sizeof(uint));
  file->seek(readValue + file->pos());

  m_contexts.loadData(file);
  m_declarations.loadData(file);

#ifdef USE_MMAP
  
  m_mappedData = file->map(file->pos(), file->size() - file->pos());
  if(m_mappedData) {
    m_mappedFile = file;
    m_mappedDataSize = file->size() - file->pos();
    file->close(); //Close the file, so there is less open file descriptors(May be problematic)
  }else{
    kDebug() << "Failed to map" << fileName;
  }
  
#endif
  
  if(!m_mappedFile) {
    QByteArray data = file->readAll();
    m_data.append(qMakePair(data, (uint)data.size()));
    delete file;
  }

  m_dataLoaded = true;
}

TopDUContext* TopDUContextDynamicData::load(uint topContextIndex) {
  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  KStandardDirs::makeDir(baseDir);

  QString fileName = baseDir + '/' + QString("%1").arg(topContextIndex);
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly)) {
    if(file.size() == 0) {
      kWarning() << "Top-context file is empty" << fileName;
      return 0;
    }
    QVector<ItemDataInfo> contextDataOffsets;
    QVector<ItemDataInfo> declarationDataOffsets;

    uint readValue;
    file.read((char*)&readValue, sizeof(uint));
    //now readValue is filled with the top-context data size
    QByteArray topContextData = file.read(readValue);

    DUChainBaseData* topData = reinterpret_cast<DUChainBaseData*>(topContextData.data());
/*    IndexedString language = static_cast<TopDUContextData*>(topData)->m_language;
    if(!language.isEmpty()) {*/
      ///@todo Load the language if it isn't loaded yet, problem: We're possibly not in the foreground thread!
//     }
    TopDUContext* ret = dynamic_cast<TopDUContext*>(DUChainItemSystem::self().create(topData));
    if(!ret) {
      kWarning() << "Cannot load a top-context, the requered language-support is probably not loaded";
      return 0;
    }

    //Disable the updating flag on loading. Sometimes it may be set when the context was saved while it was updated
    ret->setFlags( (TopDUContext::Flags) (ret->flags() & (~TopDUContext::UpdatingContext)) );

    TopDUContextDynamicData& target(*ret->m_dynamicData);

//     kDebug() << "loaded" << ret->url().str() << ret->ownIndex() << "import-count:" << ret->importedParentContexts().size() << ret->d_func()->m_importedContextsSize();

    target.m_data.clear();
    target.m_dataLoaded = false;
    target.m_onDisk = true;
    ret->rebuildDynamicData(0, topContextIndex);
    target.m_topContextData.append(qMakePair(topContextData, (uint)0));

//     kDebug() << "loaded" << ret->url().str() << ret->ownIndex() << "import-count:" << ret->importedParentContexts().size() << ret->d_func()->m_importedContextsSize();

    return ret;
  }else{
    return 0;
  }
}

bool TopDUContextDynamicData::isOnDisk() const {
  return m_onDisk;
}

void TopDUContextDynamicData::deleteOnDisk() {
  if(!isOnDisk())
    return;
  kDebug() << "deleting" << m_topContext->ownIndex() << m_topContext->url().str();

  if(!m_dataLoaded)
    loadData();

  m_contexts.deleteOnDisk();
  m_declarations.deleteOnDisk();

  m_topContext->makeDynamic();

  m_onDisk = false;

  bool successfullyRemoved = QFile::remove(filePath());
  Q_UNUSED(successfullyRemoved);
  Q_ASSERT(successfullyRemoved);
  kDebug() << "deletion ready";
}

QString KDevelop::TopDUContextDynamicData::filePath() const {
  QString baseDir = globalItemRepositoryRegistry().path() + "/topcontexts";
  KStandardDirs::makeDir(baseDir);
  return baseDir + '/' + QString("%1").arg(m_topContext->ownIndex());
}

bool TopDUContextDynamicData::hasChanged() const
{
  return !m_onDisk || m_topContext->d_ptr->m_dynamic || m_contexts.itemsHaveChanged() || m_declarations.itemsHaveChanged();
}

void TopDUContextDynamicData::store() {
//   kDebug() << "storing" << m_topContext->url().str() << m_topContext->ownIndex() << "import-count:" << m_topContext->importedParentContexts().size();

  //Check if something has changed. If nothing has changed, don't store to disk.
  bool contentDataChanged = hasChanged();
  if (!contentDataChanged) {
    return;
  }

    ///@todo Save the meta-data into a repository, and only the actual content data into a file.
    ///      This will make saving+loading more efficient, and will reduce the disk-usage.
    ///      Then we also won't need to load the data if only the meta-data changed.
  if(!m_dataLoaded)
    loadData();
  
  ///If the data is mapped, and we re-write the file, we must make sure that the data is copied out of the map,
  ///even if only metadata is changed.
  ///@todo If we split up data and metadata, we don't need to do this
  if(m_mappedData)
    contentDataChanged = true;

  m_topContext->makeDynamic();
  m_topContextData.clear();
  Q_ASSERT(m_topContext->d_func()->m_ownIndex == m_topContext->ownIndex());

  uint topContextDataSize = DUChainItemSystem::self().dynamicSize(*m_topContext->d_func());
  m_topContextData.append( qMakePair(QByteArray(DUChainItemSystem::self().dynamicSize(*m_topContext->d_func()), topContextDataSize), (uint)0) );
  uint actualTopContextDataSize = 0;

  if (contentDataChanged) {
    //We don't need these structures any more, since we have loaded all the declarations/contexts, and m_data
    //will be reset which these structures pointed into
    //Load all lazy declarations/contexts

    const auto oldData = m_data; //Keep the old data alive until everything is stored into a new data structure

    m_data.clear();

    uint newDataSize = 0;
    foreach(const ArrayWithPosition &array, oldData)
        newDataSize += array.second;

    if(newDataSize < 10000)
      newDataSize = 10000;

    //We always put 1 byte to the front, so we don't have zero data-offsets, since those are used for "invalid".
    uint currentDataOffset = 1;
    m_data.append( qMakePair(QByteArray(newDataSize, 0), currentDataOffset) );

    m_itemRetrievalForbidden = true;

    m_contexts.storeData(currentDataOffset, oldData);
    m_declarations.storeData(currentDataOffset, oldData);

    m_itemRetrievalForbidden = false;
  }

    saveDUChainItem(m_topContextData, *m_topContext, actualTopContextDataSize);
    Q_ASSERT(actualTopContextDataSize == topContextDataSize);
    Q_ASSERT(m_topContextData.size() == 1);
    Q_ASSERT(!m_topContext->d_func()->isDynamic());

    unmap();
    
    QFile file(filePath());
    if(file.open(QIODevice::WriteOnly)) {

      file.resize(0);
      
      file.write((char*)&topContextDataSize, sizeof(uint));
      foreach(const ArrayWithPosition& pos, m_topContextData)
        file.write(pos.first.constData(), pos.second);

      uint writeValue = m_contexts.offsets.size();
      file.write((char*)&writeValue, sizeof(uint));
      file.write((char*)m_contexts.offsets.data(), sizeof(ItemDataInfo) * m_contexts.offsets.size());

      writeValue = m_declarations.offsets.size();
      file.write((char*)&writeValue, sizeof(uint));
      file.write((char*)m_declarations.offsets.data(), sizeof(ItemDataInfo) * m_declarations.offsets.size());

      foreach(const ArrayWithPosition& pos, m_data)
        file.write(pos.first.constData(), pos.second);

      m_onDisk = true;

      if (file.size() == 0) {
        kWarning() << "Saving zero size top ducontext data";
      }
      file.close();
    } else {
      kWarning() << "Cannot open top-context for writing";
    }
//   kDebug() << "stored" << m_topContext->url().str() << m_topContext->ownIndex() << "import-count:" << m_topContext->importedParentContexts().size();
}

TopDUContextDynamicData::ItemDataInfo TopDUContextDynamicData::writeDataInfo(const ItemDataInfo& info, const DUChainBaseData* data, uint& totalDataOffset) {
  ItemDataInfo ret(info);
  Q_ASSERT(info.dataOffset);
  int size = DUChainItemSystem::self().dynamicSize(*data);
  Q_ASSERT(size);

  if(m_data.back().first.size() - int(m_data.back().second) < size)
      //Create a new m_data item
      m_data.append( qMakePair(QByteArray(size > 10000 ? size : 10000, 0), 0u) );

  ret.dataOffset = totalDataOffset;

  uint pos = m_data.back().second;
  m_data.back().second += size;
  totalDataOffset += size;

  DUChainBaseData& target(*reinterpret_cast<DUChainBaseData*>(m_data.back().first.data() + pos));
  memcpy(&target, data, size);

  verifyDataInfo(ret, m_data);

  return ret;
}

uint TopDUContextDynamicData::allocateDeclarationIndex(Declaration* decl, bool temporary)
{
  return m_declarations.allocateItemIndex(decl, temporary);
}

bool TopDUContextDynamicData::isDeclarationForIndexLoaded(uint index) const
{
  return m_dataLoaded && m_declarations.isItemForIndexLoaded(index);
}

bool TopDUContextDynamicData::isContextForIndexLoaded(uint index) const {
  return m_dataLoaded && m_contexts.isItemForIndexLoaded(index);
}

uint TopDUContextDynamicData::allocateContextIndex(DUContext* context, bool temporary)
{
  return m_contexts.allocateItemIndex(context, temporary);
}

bool TopDUContextDynamicData::isTemporaryContextIndex(uint index) const {
  return !(index < (0x0fffffff/2));
}

bool TopDUContextDynamicData::isTemporaryDeclarationIndex(uint index) const {
  return !(index < (0x0fffffff/2));
}

DUContext* TopDUContextDynamicData::getContextForIndex(uint index) const
{
  if(!m_dataLoaded)
    loadData();

  if (index == 0) {
    return m_topContext;
  }

  return m_contexts.getItemForIndex(index);
}

Declaration* TopDUContextDynamicData::getDeclarationForIndex(uint index) const
{
  if(!m_dataLoaded)
    loadData();

  return m_declarations.getItemForIndex(index);
}

void TopDUContextDynamicData::clearDeclarationIndex(Declaration* decl)
{
  m_declarations.clearItemIndex(decl, decl->m_indexInTopContext);
}

void TopDUContextDynamicData::clearContextIndex(DUContext* context)
{
  m_contexts.clearItemIndex(context, context->m_dynamicData->m_indexInTopContext);
}
