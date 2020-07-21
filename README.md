# CompoundFs

### Several Files in One 
CompoundFs is a fully fledged file-system organized in a single Operating-System file. It consists of a user defined 
directory structure alongside potentially multiple user-supplied data files. The whole compound file structure can easily 
be moved or send around as it exists as a single OS file.  

### Transactional Write-Operations
Write-operations either succeed entirely or appear as if they never have happend at all.
Application crashes, power-failures or file-system exceptions can not cripple existing file-data as 
file meta data is only manipulated by successfull transactions. Incomplete transactions are ignored and eventually 
rolled back.

### Two Phase Write-Operations
A write-operation consists of a first phase where any number of directory manipulations and file creation/append/delete 
or bulk-write operations can take place. Gigabytes of data can be written during this phase.  
The writer's changes are at first only visible to the writer itself while multiple readers still see the state 
of the file before the write-operation.  
The end of the write-operation employs a short commit-phase which makes the entire write-operation visible in one go. 
During the commit-phase the writer has exclusive access to the file and readers have to wait for the writer to complete.

### High Performance Directory Structure
The internal file structure is organized as a B+Tree to ensure short access times. Key-value pairs have dynamic size to
pack the maximum amount of data into the tree's nodes. 

## General Organization

- File meta data is organized in pages of 4096 bytes which is exptected to be the atomic size for hardware write-operations 
to external storage.
- Pages can be in one  of three states: `Read, New, DirtyRead`. 
  - `Read` pages where brought into memory because we needed  their information.
  - `New` pages did not exist before.
  - `DirtyRead` pages were first brought into memory as `Read` pages but need to be updated by the 
current write operation.
- The `CacheManager` manages the pages and keeps track of the memory consumption.
- At any time at most one write-operation can be active in parallel with an arbitrary number of read-operations.
- No data is ever changed on existing pages except at the very end of the write-operation the so called *Commit Phase*.
- `New` pages are allocated either by extending the file at the end or by reusing pages that were previously in-use but 
subsequently deleted.
- Deleted pages are organized as one big list of free pages in the `FreeStore`.
- A file can only grow if the `FreeStore` has no free pages left.  
- If a write-operation exceeds a predefined maximum memory limit the `CacheManager` attemps to free previously cached
pages according to the following cache eviction strategy:
### Cache Eviction Strategy
Memory occupied by pages can be reused if the page is currently not directly referenced (i.e. the page is *not hot* which is 
determined by a reference counting scheme).  
Pages are prioretized according to the costs they incure when they are needed again at a later point in time. 
This is why the `CacheManager` first tries to free pages with the lowest priority. The more memory the `CacheManager` 
has at its disposal the better its performance. Avoiding page-evictions altogether results therefore in best performance.

Page State | Priority | Evication Strategy | Future Cost
-----------| -------- |------------------- | -----------  
`Read` | 0 | Just release the memory. | Needs to be read-in again if ever needed at a later stage.
`New` | 1 | Write the page to disk before releasing it. | Needs to be read-in and potentially written again if it will be updated later on. 
`DirtyRead` | 2 | Write to disk to a previously unused location. | Same cost as for `New` pages but incures one more write-operation during the *commit-phase* when it needs to update the original page

## The Locking Protocol  
R: the Reader lock.  
W: the Writer lock.  
X: the eXclusive commit state lock.  


[] | R locked | W locked | X locked
-- | ----- | ----- | -----
**R request** | X | X | -
**W request** | X | - | - 
**X request** | - | X | -


## The Commit Protocol  

### The Log File  
- During the commit phase log records are written to the file.
- It consistes of `LogPage` pages which store a list of pairs `{ OriginalPageIndex, CopyPageIndex }`
- `LogPage` pages are at the very end of the file.
- `LogPage`pages can be savely identified


### The Commit Phase  

For the baseline we consider the most complex situation: `CacheManager` evicted some `DirtyRead` 
pages to temporary new locations.

1. Aquire eXclusive File Lock
2. Collect all free pages including the `DirtyRead` pages from `CacheManager` and mark them as free in `FreeStore`.
3. Close the `FreeStore`. From now on new pages are allocated by growing the file.
4. FSize = current file size
2. Write all `New` pages.
5. Copy original `DirtyRead` pages to new location (by growing the file)
6. Write `LogPage` pages by growing the file
7. Flush all pages. 
8. Copy new `DirtyRead` contents over original pages
9. Flush all pages
10. Cut the file size to FSize (throw away `DirtyRead` copies and `LogPage` pages)
12. Release eXclusive File Lock

### The Rollback Procedure  

If a write-operation gets interrupted before the commit-phase just cut the file to FSize and be done.  
Up to point 7. in the commit-phase no meta-data page was changed. Again just cut the file.  
After point 7. we use the LogPage info to restore the original `DirtyRead` pages and then cut the file.  

```
if (current file size == FSize)
  return;
if (last page is not LogPage)
  cut file to FSize; 
  return;
if (LogPage is not complete)
  cut file to FSize; 
  return;
foreach pair {origIdx, copyIdx} in LogPage
  copyPage from copyIdx to origIdx
cut file to FSize; 
return;

```