/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsEventQueue_h__
#define nsEventQueue_h__

#include <stdlib.h>
#include <vector>
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsIRunnable.h"
#include "nsCOMPtr.h"
#include "mozilla/AlreadyAddRefed.h"

class nsThreadPool;

// A threadsafe FIFO event queue...
class nsEventQueue
{
public:
  typedef mozilla::MutexAutoLock MutexAutoLock;

  explicit nsEventQueue(mozilla::Mutex& aLock);
  ~nsEventQueue();

  // This method adds a new event to the pending event queue.  The queue holds
  // a strong reference to the event after this method returns.  This method
  // cannot fail.
  void PutEvent(nsIRunnable* aEvent, MutexAutoLock& aProofOfLock);
  void PutEvent(already_AddRefed<nsIRunnable>&& aEvent,
                MutexAutoLock& aProofOfLock);

  //@MODIFY BEGIN 10/17/2016
  void PutEvent(nsIRunnable* aEvent, MutexAutoLock& aProofOfLock, uint64_t expTime);
  void PutEvent(already_AddRefed<nsIRunnable>&& aEvent,
                MutexAutoLock& aProofOfLock, uint64_t expTime);
  //@MODIFY END

  // This method gets an event from the event queue.  If mayWait is true, then
  // the method will block the calling thread until an event is available.  If
  // the event is null, then the method returns immediately indicating whether
  // or not an event is pending.  When the resulting event is non-null, the
  // caller is responsible for releasing the event object.  This method does
  // not alter the reference count of the resulting event.
  bool GetEvent(bool aMayWait, nsIRunnable** aEvent,
                MutexAutoLock& aProofOfLock);

  //@MODIFY BEGIN 10/17/2016
  bool GetEvent(bool aMayWait, nsIRunnable** aEvent,
                MutexAutoLock& aProofOfLock, uint64_t* expTime);
  //@MODIFY END

  //@MODIFY Sat 22 Oct 2016 03:05:10 PM EDT START
  bool getIsMain() {
    return mIsMain;
  }
  bool setIsMain(bool aIsMain) {
    //if(aIsMain)printf("isMain\n");
    return mIsMain = aIsMain;
  }
  //@MODIFY Sat 22 Oct 2016 03:06:43 PM EDT END

  //@MODIFY Tue 18 Oct 2016 11:19:12 AM EDT START
  //This function is used for find a flag runnable and swap this flag runnable with a
  //real runnable. This function will work together with GetSetFlag() function
  bool SecSwapRunnable(nsIRunnable* runnable, uint64_t expTime, MutexAutoLock& aProofOfLock);
  //@MODIFY Tue 18 Oct 2016 11:20:10 AM EDT END


  // This method returns true if there is a pending event.
  bool HasPendingEvent(MutexAutoLock& aProofOfLock)
  {
    return GetEvent(false, nullptr, aProofOfLock);
  }

  // This method returns the next pending event or null.
  bool GetPendingEvent(nsIRunnable** aRunnable, MutexAutoLock& aProofOfLock)
  {
    return GetEvent(false, aRunnable, aProofOfLock);
  }

  size_t Count(MutexAutoLock&);

private:


  //@MODIFY Sat 22 Oct 2016 03:06:53 PM EDT START
  bool mIsMain = false;
  //@MODIFY Sat 22 Oct 2016 03:06:55 PM EDT END

  //@MODIFY Tue 18 Oct 2016 11:23:11 AM EDT START

  nsIRunnable** GetSetFlag(const uint64_t expTime, int flag) {
    Page* head = mHead;
    int offset = mOffsetHead;
    if(head == NULL)return NULL;
    int qsize = 1;
    while(head != mTail || offset != mOffsetTail) {
      qsize++;
      if(offset == EVENTS_PER_PAGE) {
        offset = 0;
        head = head->mNext;
      }else {

        if(head->mExpTime[offset] == expTime) {
          //@MODIFY Sun 23 Oct 2016 04:41:10 PM EDT START
          head->mExpTime[offset] = (expTime >> 1 << 1);
          //head->mExpTime[offset] = (expTime >> 1 << 1 | flag);
          //@MODIFY Sun 23 Oct 2016 04:41:13 PM EDT END

          //nsIRunnable*& queueLocation = head->mEvents[offset];
          //printf("%d\n",qsize);
          return &(head->mEvents[offset]);
        }
        offset ++;
      }
    }
    //printf("%d\n",qsize);
    return NULL;
  }
  //@MODIFY Tue 18 Oct 2016 11:39:38 AM EDT END

//@MODIFY Thu 13 Oct 2016 02:55:25 PM EDT START

  void GetRunNow() {
    if(IsEmpty()) return ;
    Page* head = mHead, *tail = mTail, *nextRun = NULL;
    uint16_t offset = -1;
    int first_end = 0;
    uint64_t minExpTime = 2147483648;// larger than the normal int
    const bool debug = 0;
    std::vector<uint64_t > debugQueue;
    //printf("%lld headheadhead\n", mHead->mEvents[mOffsetHead]->expTime);

    if(head != tail) first_end = EVENTS_PER_PAGE;
    else first_end = mOffsetTail;
    for(int i = mOffsetHead;i < first_end;++ i){
      debugQueue.push_back(head->mExpTime[i]);
      if(head->mExpTime[i] < minExpTime){
        nextRun = head;
        offset = i;
        minExpTime = head->mExpTime[i];
      }
    }

    if(head != tail) head = head->mNext;

    while(head != tail) {
      for(int i = 0;i < EVENTS_PER_PAGE;++ i){
        debugQueue.push_back(head->mExpTime[i]);
        if(head->mExpTime[i] < minExpTime){
          nextRun = head;
          offset = i;
          minExpTime = head->mExpTime[i];
        }
      }
      head = head->mNext;
    }

    if(mHead != mTail) {
      for(int i = 0;i < mOffsetTail;++ i) {
        debugQueue.push_back(tail->mExpTime[i]);
        if(tail->mExpTime[i] < minExpTime){
          nextRun = tail;
          offset = i;
          minExpTime = head->mExpTime[i];
        }
      }
    }

    if(offset < EVENTS_PER_PAGE && (mHead != head || mOffsetHead != offset)) {

      nsIRunnable* tmp = mHead->mEvents[mOffsetHead];
      mHead->mEvents[mOffsetHead] = nextRun->mEvents[offset];
      nextRun->mEvents[offset] = tmp;

      int expTime = mHead->mExpTime[mOffsetHead];
      mHead->mExpTime[mOffsetHead] = nextRun->mExpTime[offset];
      nextRun->mExpTime[offset] = expTime;

      if(debug) {
        for(auto it = debugQueue.begin(); it != debugQueue.end();++ it) {
          printf("%lu ",*it);
        }
        printf("Current %lu %lu \n", mHead->mExpTime[mOffsetHead], head->mExpTime[offset]);
      }
    }

    //bool f = mHead->flag[mOffsetHead];
    //mHead->flag[mOffsetHead] = nextRun->flag[offset];
    //nextRun->flag[offset] = t;
  }
  //@MODIFY Thu 13 Oct 2016 02:55:29 PM EDT END

  uint64_t curTime = 1e8;

  bool IsEmpty()
  {
    return !mHead || (mHead == mTail && mOffsetHead == mOffsetTail);
  }

  enum
  {
    EVENTS_PER_PAGE = 255
  };

  // Page objects are linked together to form a simple deque.

  struct Page
  {
    struct Page* mNext;
    //@MODIFY Mon 17 Oct 2016 07:13:40 PM EDT START
    //bool flag[EVENTS_PER_PAGE + 1];
    uint64_t mExpTime[EVENTS_PER_PAGE + 1];
    //@MODIFY Mon 17 Oct 2016 07:13:43 PM EDT END

    nsIRunnable* mEvents[EVENTS_PER_PAGE];
  };

  static_assert((sizeof(Page) & (sizeof(Page) - 1)) == 0,
      "sizeof(Page) should be a power of two to avoid heap slop.");

  static Page* NewPage()
  {
    return static_cast<Page*>(moz_xcalloc(1, sizeof(Page)));
  }

  static void FreePage(Page* aPage)
  {
    free(aPage);
  }

  Page* mHead;
  Page* mTail;

  uint16_t mOffsetHead;  // offset into mHead where next item is removed
  uint16_t mOffsetTail;  // offset into mTail where next item is added
  mozilla::CondVar mEventsAvailable;

  // These methods are made available to nsThreadPool as a hack, since
  // nsThreadPool needs to have its threads sleep for fixed amounts of
  // time as well as being able to wake up all threads when thread
  // limits change.
  friend class nsThreadPool;
  void Wait(PRIntervalTime aInterval)
  {
    mEventsAvailable.Wait(aInterval);
  }
  void NotifyAll()
  {
    mEventsAvailable.NotifyAll();
  }
};

#endif  // nsEventQueue_h__
