Avraam Tsantekidis : 1969
Vaggelis Xristelis : 1977
Xristos Tsavas	   : 1967

__________________________________________________________________________________

The following license is applied to all the custom code written above the sigmod contest template,
the code that is licensed under the sigmod kaust license remains as is.

LICENSE

Copyright (c) 2013 Avraam Tsantekidis, Vaggelis Xristelis, Xristos Tsavas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
___________________________________________________________________________________

SUBMISSION

This folder contains multiple implementation of the project that should work, but the main
version that should be taken into consideration is the file "core.cpp"

___________________________________________________________________________________

CODE EXPLANATION

InitializeIndex() : Using the constant declared at the beginning of the file MAX_THREAD_COUNT,
this function creates that amount of threads that are going to receive jobs from the function
MatchDocument(). It Creates a new object from the class Job for each thread and gives it as an
arguement for the threads starting function DoWork(). We also keep the pointer in an array to 
job sumbissions to the specific threads. Also in the beginning of the function we unlock the
mutexes we will use as an initialization.

StartQuery() : Adds all incoming queries to a queue and sets the variable hasqueries to true,
meaning that there are new queries to be added/deleted to/from each thread query containers and then
returns.

EndQuery() : Adds all queries to be destroyed in a queue and sets the variable hashqueries to true, 
meaning that there are new queries to be added/deleted to/from each thread query containers and then
returns.

*free threads means a thread that has done the work it has been assigned
MatchDocument() : Begins by checking if there are any queries to be added/deleted (using the
bool variable hasqueries). if there are, it starts a loop searching a free thread to assign 
it the job of adding and deleting all the queries that need to be added or deleted. After 
it assigns the work to a free thread, it keeps searching the threads for a free one to assign
the document analysis. Once it finds one it submits the document to match and returns. 

GetNextAvailRes(,,) : This function remains almost identical to the template one. The only
difference is the multithreading protection from simultaneous read/write to the same document
result container, using mutexes.

class Job : this is used so we a common object (either a query or a document) to pass to each
each thread for proccessing.

The function tha is used as the thread initializer
dowork(Job *j) : Each thread begins in a locked state (by double locking a mutex specific to it),
waiting for a job. Whenever a job arrives, it checks if it has to do with adding/deleting a query
or with matching a document to the current queries. If the jobtype of the job it has is 
adding/deleting a query it procceeds by adding to every thread specific list of queries to be
added/deleted, using a program wide mutex to avoid concurrency issues and to ensure that all the
threads will have the latest queries before matching a document (queries are tokenized to their
single words). On the other hand if the jobtype is matching a document, it begins by checking 
its list with the new queries to add/delete, and if it is not empty, it updates its hashtable of
queries to the current version that will be used to match the inbound document.
	After updating the queries, the thread begins by tokenizing the document and adding each
of the words/tokens to a hashset (so as to prevent duplicate checking), and if it is unique, it
is inserted in the trie data structure. While doing that we keep the maximum and the minimum 
length of the document's words so we iterate through the query hashtables that are just enough
and not more. For every query we check, we set some optimizers depending on the distances of 
words to avoid double checking the same word for different queries.
	Finishing the matching of the document to the current queries, all the queries that
were validated are added to an array and inserted into a vector of documents (using a mutex to
prevent inserting/deleting concurrently). Afterwards it cleans up the local variables of the thread
and locks the its mutex waiting for the next job.

Further thread functionality explanation : Every thread locks itself using a thread specific
mutex, when it is done with the job it has to do, in order to prevent either having it do an
endless loop which would consume resources or letting it finish execution and spin a new thread
for the each job. This improved the performance considerably considering the previous two methods.

For searching exact distance multiple hashtables are used depending on the size of the word in it.

For searching hamming and edit distance a prefix trie is used on the documents words (The 
implemantation for the edit distance using the prefix trie was found online and the hamming 
distance was added as an expansion to the prefix trie. Reference to the original :
http://murilo.wordpress.com/2011/02/01/fast-and-easy-levenshtein-distance-using-a-trie-in-c/)

On the prefix trie there is a length check implemented using the bits of an 32-bit integer, used
to determine if there is a word with the same lentgh as the one we are looking for below the current
branch of the trie.

Any other detail about this project would be too technical for a simple document explaining the funtionality. Thank you for reading this!
