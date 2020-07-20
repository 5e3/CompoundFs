# CompoundFs

### Several Files in One 
CompoundFs is a fully fledged file-system organized in a single Operating-System file. It consists of a user defined 
directory structure alongside potentially multiple user-supplied data files. The whole compound file structure can easily 
be moved or send around as it exists as a single OS file.  

### Transactional Write-Operations
Write-operations either succeed entirely or appear as if they have never happend at all.
Application crashes, power-failures or file-system exceptions can never cripple existing file-data. 
File meta data is only manipulated by successfull transactions. Incomplete transactions are ignored and eventually 
rolled back.

### Two Phase Write-Operations
A write-operation consists of a first phase were any number of directory manipulations and file creation/append/delete 
or bulk-write operations can take place. Gigabytes of data can be written during this phase.  
The writer's changes are at first only visible to the writer itself while multiple readers still see the state 
of the file before the beginning of the write-operation.  
The end of the write-operation employs a short commit-phase which makes the entire write-operation visible. 
During the commit-phase the writer has exclusive access to the file and readers have to wait for the writer to complete.

# General Organization

- All data is organized in pages of 4096 bytes which is exptected to be the atomic size for hardware write-operations 
to external storage.
- Pages can be in any of three states: `Read, New, DirtyRead`. 
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


# The Commit Protocol
All data is organized in pages of 4096 bytes size which is exptected to be the atomic size for hardware
write operations. :
The `CacheManager` manages the pages