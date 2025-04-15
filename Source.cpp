#include <windows.h>
#include <iostream>
#include <fstream>

#define MAX_COUNT 50
#define MAX_THREADS 6
#define PAIR_SIZE 2
#define NUM_PAIRS (MAX_THREADS / PAIR_SIZE)

HANDLE hPairEvents[NUM_PAIRS];       // One event per pair
int pairDoneCount[NUM_PAIRS] = { 0 };  // Count finished threads in each pair

HANDLE hSyncEvent = CreateEvent(
    NULL,               // default security attributes
    TRUE,               // manual-reset event
    TRUE,              // initial state is nonsignaled
    TEXT("SyncEvent")  // object name
);

CRITICAL_SECTION cs;
std::ofstream file("example.txt");

void PrintNums(int _threadIndex) {
    if (_threadIndex % 2 == 0) {
        for (int i = 1; i <= MAX_COUNT; i++) {
            file << i << std::endl;
            std::cout << i << " ";
        }
    }
    else {
        for (int i = -1; i >= -MAX_COUNT; i--) {
            file << i << std::endl;
            std::cout << i << " ";
        }
    }
}

void NoSync(int _threadIndex) {
    PrintNums(_threadIndex);
}

void EventSync(int _threadIndex, HANDLE _hSyncEvent) {
    WaitForSingleObject(_hSyncEvent, INFINITE);
    ResetEvent(_hSyncEvent);
    PrintNums(_threadIndex);
    SetEvent(_hSyncEvent);
}

void CritSectSync(int _threadIndex, CRITICAL_SECTION *cs) {
    EnterCriticalSection(cs);
    PrintNums(_threadIndex);
    LeaveCriticalSection(cs);
}

DWORD WINAPI ThreadFunc(LPVOID lpParam) {
    int threadIndex = int(lpParam);
    int pairIndex = threadIndex / PAIR_SIZE;

    // Wait for this thread's pair to be allowed to run
    WaitForSingleObject(hPairEvents[pairIndex], INFINITE);

    std::cout << "\nThread " << threadIndex << " is executing...\n";
    Sleep(500);  // Simulate delay

    switch (pairIndex)
    {
    case 0:
        NoSync(threadIndex);
        break;
    case 1:
        EventSync(threadIndex, hSyncEvent);
        break;
    case 2:
        CritSectSync(threadIndex, &cs);
        break;
    default:
        break;
    }
    
    pairDoneCount[pairIndex]++;
    if (pairDoneCount[pairIndex] == PAIR_SIZE && pairIndex + 1 < NUM_PAIRS) {
        // Signal the next pair only when both threads in the current pair are done
        SetEvent(hPairEvents[pairIndex + 1]);
    }

    return 0;
}

int main() {
    InitializeCriticalSection(&cs);

    HANDLE hThreads[MAX_THREADS];

    // Create events for each pair
    for (int i = 0; i < NUM_PAIRS; i++) {
        hPairEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    }

    // Start with the first pair
    SetEvent(hPairEvents[0]);

    // Create threads
    for (int i = 0; i < MAX_THREADS; i++) {
        hThreads[i] = CreateThread(NULL, 0, ThreadFunc, (LPVOID)i, CREATE_SUSPENDED, NULL);
    }

    for (auto i : hThreads) { ResumeThread(i); }
    // Wait for all threads
    WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);

    // Cleanup
    for (int i = 0; i < MAX_THREADS; i++) {
        CloseHandle(hThreads[i]);
    }
    for (int i = 0; i < NUM_PAIRS; i++) {
        CloseHandle(hPairEvents[i]);
    }

    DeleteCriticalSection(&cs);
    file.close();

    return 0;
}
