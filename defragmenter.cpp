#include <iostream>
#include "defragmenter.h"
#include "mynew.h"
#include "DefragRunner.h"
#include "QuadraticProbing.h"
#include "BinaryHeap.h"
using namespace std;


Defragmenter::Defragmenter(DiskDrive * diskDrive) {
  
  int fileNum = diskDrive->getNumFiles();
  DiskBlock * notfound = NULL;
  
  int tableSize = 5400;
  QuadraticHashTable<DiskBlock*> * storage;
  storage = new QuadraticHashTable<DiskBlock*>(notfound, tableSize);
  QuadraticHashTable<unsigned> * indexHash;
  indexHash = new QuadraticHashTable<unsigned>(-1, 23000);

  BinaryHeap <int> * heap;
  unsigned maxEmpty = 200000;
  heap = new BinaryHeap <int> (maxEmpty + 1);
  
  unsigned count = 0;
  unsigned k = diskDrive->getCapacity() - 1;
  for (; k >= 2 && count <= (float)maxEmpty/1.5; k--) {
    if (diskDrive->FAT[k] == false) { // unused
      heap->insert(-k);
      count++;
    }
  }
  
  unsigned min = (diskDrive->getCapacity() + k) / 2;
  
  unsigned index = 2;
  DiskBlock * fileHead = NULL;
  DiskBlock * move = NULL;
  
  unsigned currentPos = 0;
  
  for (int i = 0; i < fileNum ; i++) {
    unsigned blockID = diskDrive->directory[i].getFirstBlockID();
    
    if (blockID == diskDrive->directory[i].getFirstBlockID()) { // first
      diskDrive->directory[i].setFirstBlockID(index);
    }
    
    do {
      
      if (blockID < index) { // read from storage
        fileHead = storage->find(blockID);
        if (fileHead) {
          storage->remove(blockID);

        }
        else { // read from disk
          currentPos = indexHash->find(blockID);

          while (indexHash->find(currentPos) != (unsigned)-1) {
            indexHash->remove(blockID);
            blockID = currentPos;
            currentPos = indexHash->find(blockID);
          }
          
          fileHead = diskDrive->readDiskBlock(currentPos);
          if (currentPos != index) {
            diskDrive->FAT[currentPos] = false;
            if (currentPos > min && heap->getSize() < maxEmpty)
              heap->insert(-currentPos);
            
          }
          indexHash->remove(blockID);
        }
        
      }
      else { // read from disk
        fileHead = diskDrive->readDiskBlock(blockID);
        if (blockID != index) {
          diskDrive->FAT[blockID] = false;
          if (currentPos > min && heap->getSize() < maxEmpty) {
            heap->insert(-blockID);
            
          }
        }
      }
      
      if (blockID != index) { // need to write to index
        if (diskDrive->FAT[index]) { // if FAT = true = used, move it to storage
          move = diskDrive->readDiskBlock(index);

          if (!storage->needRehash()) {
            storage->insert(index, move);
          }
          
          else { // put it in indexHash

            k = -heap->findMin();
            heap->deleteMin();

            diskDrive->writeDiskBlock(move, k);

            indexHash->insert(index, k);
            diskDrive->FAT[k] = true;
            delete move;
          }
        }
        
        blockID = fileHead->getNext();
        if (blockID != 1) {
          fileHead->setNext(index + 1);
        }
        diskDrive->writeDiskBlock(fileHead, index);
        diskDrive->FAT[index] = true;
        delete fileHead;
      }
      else { // blockID == index
        blockID = fileHead->getNext();
      }
      index++;

    }
    while (blockID != 1);
  }

  delete storage;
  delete indexHash;
  delete heap;
  
}