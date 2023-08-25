# Technical Details of the File Locking Protocol

### Windows / Linux
For Windows we use the default mandatory file locks (LockFileEx()). Mandatory means
that trying to read/write to a file where you don't hold the correct lock will
fail. 

Linux file locking is implemented in terms of advisory File Descripter Locks. 
Advisory locks will not prevent you from reading or writing.


### File Locks
File locks generally allow you to request either a shared lock, which can be 
granted to several readers at the same time or an exclusive lock which allows one 
single writer to get access. With a file lock there is always a range of bytes 
associated. We can use several such ranges at the same time. It is even 
possible to lock regions outside the file.


### Locking use case
Locking protocol is designed to allow a single writer to store the bulk of its 
data in the file without affecting readers. During that phase the writer holds
the `WriteLock` and readers hold their respective `ReadLock`. When the writer is
ready to commit it transforms the `WriteLock` into a `CommitLock` which will grant
exclusive access to the file. This can only happen after all readers gave up their
`ReadLock`. 

The writer now holds the `CommitLock` and the `WriteLock`. The `CommitLock` 
prevents readers to enter and the `WriteLock` blocks other writers. After commit
the writer gives up its locks thus opens access for readers or a writer 
respectively. These three lock types you will find on the API level. These
functions block the caller until the request can be granted. 

### Implementation
Internally `LockProtocol` makes use of three locks: 
1. `m_writer` which is locked exclusively when requesting the `WriteLock`
2. `m_shared` which locked in shared mode by readers and in exclusive mode when
requesting a `CommitLock`
3.  `m_gate` the gate lock:

#### Without using `m_gate`
Imagine the following scenario: Writer1 holds the `WriteLock` and is now ready
to commit but there is still Reader1 holding a `ReadLock` (`m_shared` is locked in
shared mode). Writer1 tries to get exclusive access to `m_shared` but is 
blocked because of Reader1. 

Reader2 arrives, tries to get shared access to `m_shared` and is granted because
`m_shared` is already in shared mode. Reader1 gives up its lock but Writer1 
continues to wait as Reader2 has shared access to `m_shared`.

Reader3 arrives, tries to get shared access to `m_shared` and is granted because...
This phenomenon is known as "starvation of the writer". We'll use the gate lock to
avoid that situation.

#### Use `m_gate` to establish fairness
When requesting a `ReadLock` or a `CommitLock` we first lock on `m_gate`. After 
succeeding we aquire the lock on `m_shared` and after `m_shared` is granted we
give up `m_gate`. Like this:
```
void lockShared(LockMode mode)
{
	m_gate.lock(mode);	(1)
	m_shared.lock(mode);	(2)
	m_gate.unlock();	(3)
	return;
}
```
See what happens in the above scenario: Writer1 wants to commit. It owns `m_gate` 
already but is now blocked on (2) because Reader1 holds `m_shared` in shared mode.

Reader2 arrives, tries to get `m_gate` in (1) but blocks because it is owned 
by Writer1. 

Reader1 gives up its lock, Writer1 gets access to `m_shared`, gives up `m_gate` 
and starts commiting. Reader2 gets access to `m_gate` but get blocked on `m_shared`
and must wait until Writer1 gives up its locks. Note that if at any point in time
Writer2 arrived he was blocked on `m_writer` and can only resume when Writer1 
leaves.

This way the LockProtocol becomes fair. Readers arriving after Writers requesting
the `CommitLock` must wait for the Writer to commit.

### How to Place Three Locks on One File
To have any chance that windows and linux work together the locks must be at the 
same position in the file. Moving the locks around breaks backwards compatibility.
All three locks must be placed outside the range of valid data. (The following is 
only of concern for windows with its mandatory locks). 
1. If `m_writer` is inside the valid range readers will fail to read if `m_writer`
is locked.
2. if `m_shared` is inside the valide range writers will fail to write if 
`m_shared` is locked by readers.
3. If `m_gate` is inside the valid range the problem is more subtle: When Writer1 
(as in the above scenario) is owning `m_gate` but blocks for some time on `m_shared`
readers start failing to read.

Placing all locks outside the valid range makes windows file locks advisory. Let's
therefore place the locks at the end of a valid file length that is 
`std::numeric_limits<long long>::max() - 3` with range length of one byte. Note
that the current maximum file length for CompoundFs is at ~16 Terrabytes so the 
lock ranges are far away from that.

