/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsThread_h__
#define nsThread_h__

#include "mozilla/Mutex.h"
#include "nsIThreadInternal.h"
#include "nsISupportsPriority.h"
#include "nsEventQueue.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsTObserverArray.h"
#include "mozilla/Attributes.h"
#include "mozilla/NotNull.h"
#include "nsAutoPtr.h"
#include "mozilla/AlreadyAddRefed.h"

#include <set>
#include <string>

//_MODIFY
#include <map>
#include <queue>
//_MODIFY

namespace mozilla {
class CycleCollectedJSRuntime;
}

using mozilla::NotNull;

// A native thread
class nsThread
  : public nsIThreadInternal
  , public nsISupportsPriority
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSITHREAD
  NS_DECL_NSITHREADINTERNAL
  NS_DECL_NSISUPPORTSPRIORITY
  using nsIEventTarget::Dispatch;

  //_MODIFY BEGIN 10/21/2016
  uint64_t expTime=0;

  void* key = (void*)1;

  uint64_t add=0;

  uint64_t flagExpTime=0;

  std::set<std::string> nameSet;

  std::string mName;
  
  std::map<void*, void*> keyMap;

  bool handleFlag = false;
  //_MODIFY END

    //_MODIFY
    void cancelFlag(uint64_t expTime);
    std::set<uint64_t> cancelFlags;
    std::set<uint64_t> blockEvents;
    std::queue<uint64_t> blockEventsExpTimeQueue;
    std::queue<nsCOMPtr<nsIRunnable> > blockEventsQueue;
    //_MODIFY

  enum MainThreadFlag
  {
    MAIN_THREAD,
    NOT_MAIN_THREAD
  };

  MainThreadFlag mIsMainThread;

  nsThread(MainThreadFlag aMainThread, uint32_t aStackSize);

  // Initialize this as a wrapper for a new PRThread.
  nsresult Init();

  // Initialize this as a wrapper for the current PRThread.
  nsresult InitCurrentThread();

  // The PRThread corresponding to this thread.
  PRThread* GetPRThread()
  {
    return mThread;
  }

  // If this flag is true, then the nsThread was created using
  // nsIThreadManager::NewThread.
  bool ShutdownRequired()
  {
    return mShutdownRequired;
  }

  // Clear the observer list.
  void ClearObservers()
  {
    mEventObservers.Clear();
  }

  void
  SetScriptObserver(mozilla::CycleCollectedJSRuntime* aScriptObserver);

  uint32_t
  RecursionDepth() const;

  void ShutdownComplete(NotNull<struct nsThreadShutdownContext*> aContext);

  void WaitForAllAsynchronousShutdowns();

  //_MODIFY BEGIN 10/22/2016
  void putFlag(uint64_t expTime,int flag = 0);
  //_MODIFY END

#ifdef MOZ_CRASHREPORTER
  enum class ShouldSaveMemoryReport
  {
    kMaybeReport,
    kForceReport
  };

  static bool SaveMemoryReportNearOOM(ShouldSaveMemoryReport aShouldSave);
#endif

private:
  void DoMainThreadSpecificProcessing(bool aReallyWait);

protected:
  class nsChainedEventQueue;

  class nsNestedEventTarget;
  friend class nsNestedEventTarget;

  friend class nsThreadShutdownEvent;

  virtual ~nsThread();

  bool ShuttingDown()
  {
    return mShutdownContext != nullptr;
  }

  static void ThreadFunc(void* aArg);

  // Helper
  already_AddRefed<nsIThreadObserver> GetObserver()
  {
    nsIThreadObserver* obs;
    nsThread::GetObserver(&obs);
    return already_AddRefed<nsIThreadObserver>(obs);
  }

  // Wrappers for event queue methods:
  nsresult PutEvent(nsIRunnable* aEvent, nsNestedEventTarget* aTarget);
  nsresult PutEvent(already_AddRefed<nsIRunnable> aEvent,
                    nsNestedEventTarget* aTarget);
  nsresult PutEvent(nsIRunnable* aEvent, nsNestedEventTarget* aTarget, uint64_t expTime);
  nsresult PutEvent(already_AddRefed<nsIRunnable> aEvent, nsNestedEventTarget* aTarget, uint64_t expTime);

  nsresult DispatchInternal(already_AddRefed<nsIRunnable> aEvent,
                            uint32_t aFlags, nsNestedEventTarget* aTarget);

  NS_IMETHODIMP
    Dispatch(already_AddRefed<nsIRunnable> aEvent, uint32_t aFlags, uint64_t expTime);

  nsresult
    DispatchInternal(already_AddRefed<nsIRunnable> aEvent, uint32_t aFlags,
                           nsNestedEventTarget* aTarget, uint64_t exptime);

  struct nsThreadShutdownContext* ShutdownInternal(bool aSync);

  // Wrapper for nsEventQueue that supports chaining.
  class nsChainedEventQueue
  {
  public:
    explicit nsChainedEventQueue(mozilla::Mutex& aLock)
      : mNext(nullptr)
      , mQueue(aLock)
    {
    }

    //_MODIFY BEGIN 10/17/2016
    bool GetEvent(bool aMayWait, nsIRunnable** aEvent,
                  mozilla::MutexAutoLock& aProofOfLock, uint64_t* expectedEndTime, bool* isFlag)
    {
      bool result = mQueue.GetEvent(aMayWait, aEvent, aProofOfLock, expectedEndTime);
      *isFlag = (*expectedEndTime & 1) == 1;
      *expectedEndTime = *expectedEndTime >> 1;
      return result;
    }

    void PutEvent(nsIRunnable* aEvent, mozilla::MutexAutoLock& aProofOfLock, uint64_t expectedEndTime, bool isFlag)
    {
      if(isFlag)expectedEndTime = expectedEndTime << 1 | 1;
      else expectedEndTime = (expectedEndTime << 1);
      mQueue.PutEvent(aEvent, aProofOfLock, expectedEndTime);
    }

    void PutEvent(already_AddRefed<nsIRunnable> aEvent,
                  mozilla::MutexAutoLock& aProofOfLock, uint64_t expectedEndTime, bool isFlag)
    {
      if(isFlag)expectedEndTime = (expectedEndTime << 1) + 1;
      else expectedEndTime = (expectedEndTime << 1);
      mQueue.PutEvent(mozilla::Move(aEvent), aProofOfLock,expectedEndTime);
    }

    bool SecSwapRunnable(nsIRunnable* runnable, uint64_t expTime, mozilla::MutexAutoLock& aProofOfLock){
      return mQueue.SecSwapRunnable(runnable, expTime, aProofOfLock);
    }

    bool setIsMain(bool aIsMain) {
      return mQueue.setIsMain(aIsMain);
    }
    //_MODIFY END

    bool GetEvent(bool aMayWait, nsIRunnable** aEvent,
                  mozilla::MutexAutoLock& aProofOfLock)
    {
      return mQueue.GetEvent(aMayWait, aEvent, aProofOfLock);
    }

    void PutEvent(nsIRunnable* aEvent, mozilla::MutexAutoLock& aProofOfLock)
    {
      mQueue.PutEvent(aEvent, aProofOfLock);
    }

    void PutEvent(already_AddRefed<nsIRunnable> aEvent,
                  mozilla::MutexAutoLock& aProofOfLock)
    {
      mQueue.PutEvent(mozilla::Move(aEvent), aProofOfLock);
    }

    bool HasPendingEvent(mozilla::MutexAutoLock& aProofOfLock)
    {
      return mQueue.HasPendingEvent(aProofOfLock);
    }

    nsChainedEventQueue* mNext;
    RefPtr<nsNestedEventTarget> mEventTarget;

  private:
    nsEventQueue mQueue;
  };

  class nsNestedEventTarget final : public nsIEventTarget
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIEVENTTARGET


    nsNestedEventTarget(NotNull<nsThread*> aThread,
                        NotNull<nsChainedEventQueue*> aQueue)
      : mThread(aThread)
      , mQueue(aQueue)
    {
    }

    NotNull<RefPtr<nsThread>> mThread;

    // This is protected by mThread->mLock.
    nsChainedEventQueue* mQueue;

  private:
    ~nsNestedEventTarget()
    {
    }
  };

  // This lock protects access to blockEventsmEvents and mEventsAreDoomed.
  // All of those fields are only modified on the thread itself (never from
  // another thread).  This means that we can avoid holding the lock while
  // using mObserver and mEvents on the thread itself.  When calling PutEvent
  // on mEvents, we have to hold the lock to synchronize with PopEventQueue.
  mozilla::Mutex mLock;

  //_MODIFY BEGIN 10/23/2016
  mozilla::Mutex mFlagLock;
  bool flag=false;

  bool setFlag(bool aFlag);

  bool getFlag();

  nsIRunnable* flagEvent;
  //_MODIFY END

  nsCOMPtr<nsIThreadObserver> mObserver;
  mozilla::CycleCollectedJSRuntime* mScriptObserver;

  // Only accessed on the target thread.
  nsAutoTObserverArray<NotNull<nsCOMPtr<nsIThreadObserver>>, 2> mEventObservers;

  NotNull<nsChainedEventQueue*> mEvents;  // never null
  nsChainedEventQueue mEventsRoot;

  int32_t   mPriority;
  PRThread* mThread;
  uint32_t  mNestedEventLoopDepth;
  uint32_t  mStackSize;

  // The shutdown context for ourselves.
  struct nsThreadShutdownContext* mShutdownContext;
  // The shutdown contexts for any other threads we've asked to shut down.
  nsTArray<nsAutoPtr<struct nsThreadShutdownContext>> mRequestedShutdownContexts;

  bool mShutdownRequired;
  // Set to true when events posted to this thread will never run.
  bool mEventsAreDoomed;
  //MainThreadFlag mIsMainThread;
};

#if defined(XP_UNIX) && !defined(ANDROID) && !defined(DEBUG) && HAVE_UALARM \
  && defined(_GNU_SOURCE)
# define MOZ_CANARY

extern int sCanaryOutputFD;
#endif

#endif  // nsThread_h__
