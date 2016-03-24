/*
 * core.cpp version 1.0
 * Copyright (c) 2013 KAUST - InfoCloud Group (All Rights Reserved)
 * Author: Amin Allam
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "../include/core.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <deque>
#include <set>
#include <pthread.h>
#include <atomic>

#define MAX_THREAD_COUNT 2

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

using namespace std;


void testtime(clock_t &clock1,string say)
{
  cout.precision(10);
  clock_t clock2 = clock();
  double diffticks=clock2-clock1;
  double diffms=(diffticks)/(CLOCKS_PER_SEC/100);
  cout<<say<<" "<< fixed <<diffms<<endl;
  clock1 = clock();

}
///////////////////////////////////////////////////////////////////////////////////////////////


// Computes edit distance between a null-terminated string "a" with length "na"
//  and a null-terminated string "b" with length "nb"


inline int EditDistance(char *s1, char *s2,int limit) {
  unsigned int s1len, s2len, x, y, lastdiag, olddiag;
  s1len = strlen(s1);
  s2len = strlen(s2);
  unsigned int column[s1len+1];
  for (y = 1; y <= s1len; y++)
    column[y] = y;
  for (x = 1; x <= s2len; x++) {
      column[0] = x;
      for (y = 1, lastdiag = x-1; y <= s1len; y++) {
          olddiag = column[y];
          column[y] = MIN3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
          lastdiag = olddiag;
      }
  }
  return(column[s1len]);
}

inline bool CheckEditDistance(char* a, char* b, int limit)
{
  int check = EditDistance(a, b,limit);
  if (check>limit || check<0)
    return false;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

// Computes Hamming distance between a null-terminated string "a" with length "na"
//  and a null-terminated string "b" with length "nb"

inline bool CheckHammingDistance(char* a, char* b, int nb,unsigned int limit)
{
  unsigned int num_mismatches=0;
  int j;
  for(j=0;j<nb;j++)
    if(a[j]!=b[j])
      {
        num_mismatches++;
        if (num_mismatches>limit)
          return false;
      }
  return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////

// Keeps all information related to an active query

struct SimplerQuery
{
  QueryID query_id;
  MatchType match_type;
  char str[MAX_WORD_LENGTH+1];
  unsigned int match_dist;
  bool matched = false;

  bool operator<(const SimplerQuery& rhs) const
  {
    if (match_type == MT_HAMMING_DIST)
      {
        if(rhs.match_type == MT_HAMMING_DIST)
          {
            if(match_dist<=rhs.match_dist)
              return true;
            else return false;
          }
        else return false;
      }
    else if (match_type == MT_EDIT_DIST)
      {
        if (rhs.match_type == MT_HAMMING_DIST)
          return false;
        else if (rhs.match_type == MT_EDIT_DIST)
          {
            if (match_dist<=rhs.match_dist)
              return true;
            else return false;
          }
        else return true;
      }
    else if (rhs.match_type != MT_EXACT_MATCH)
      return false;
    else return true;

  }

  bool operator==(const SimplerQuery& rhs) const
                                            {
    return query_id == rhs.query_id;
                                            }

  bool operator<(const QueryID& rhs) const
  {
    if (query_id == rhs)
      return false;
    return true;
  }

  bool operator==(const QueryID& rhs) const
                                            {
    return query_id == rhs;
                                            }
};




struct Query
{
  QueryID query_id;
  char str[MAX_QUERY_LENGTH+1];
  MatchType match_type;
  unsigned int match_dist;

  SimplerQuery toSQuery()
  {
    SimplerQuery s;
    s.query_id = query_id;
    s.match_dist = match_dist;
    s.match_type = match_type;
    return s;
  }


};

///////////////////////////////////////////////////////////////////////////////////////////////

// Keeps all query ID results associated with a document
struct Document
{
  DocID doc_id;
  unsigned int num_res;
  QueryID* query_ids;
};


///////////////////////////////////////////////////////////////////////////////////////////////

// Keeps all currently active queries

// Keeps all currently available results that has not been returned yet
std::vector<Document> docs;

//Extra stuff

struct DocumentAll
{
  DocID doc_id;
  unsigned int num_res;
  char* str;
  Document toDoc()
  {
    Document doc;
    doc.doc_id = doc_id;
    doc.num_res = num_res;
    return doc;
  }
};


typedef enum
{
  ADD_QUERY,
  DEL_QUERY,
  MATCH_DOC,
  NOTHING

}JobType;

struct IncQuery
{
  QueryID query_id;
  char str[MAX_QUERY_LENGTH+1];
  MatchType match_type;
  unsigned int match_dist;
  JobType jtype;
  SimplerQuery toSQuery()
    {
      SimplerQuery s;
      s.query_id = query_id;
      s.match_dist = match_dist;
      s.match_type = match_type;
      return s;
    }
};

struct c_cmp
{
  bool operator()(char const *c1,char const *c2) const
  {
    int p = 0;
    do
      {
        if(c1[p]!=c2[p])
          return false;
        p++;

      }while(c1[p]!='\0' && c2[p]!='\0');
    if(c1[p]!=c2[p])
      return false;
    return true;
  }
};

struct c_Hash
{
  std::size_t precision = 2;
  std::size_t operator()(const char *k) const
  {
    uint32_t hash = 0;
    for(; *k; ++k)
      {
        hash += *k;
        hash += (hash << 10);
        hash += *k;
        hash ^= (hash >> 6);
      }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += *k;
    hash += (hash << 15);
    return ((size_t)hash);
  }
};



class Job
{
public:
  int id;
  JobType jobtype;
  Query query;
  DocumentAll doc;
  std::mutex mutlock;

  deque<IncQuery> queryaddeler;
  deque<IncQuery> queryaddelerh;


  std::atomic<bool> done;
  bool finish;
  bool exited;
  Job(int id)
  {

    this->id = id;
    jobtype = NOTHING;
    done = true;
    finish = false;
    exited = false;
    doc.str = NULL;
  }

};

deque<IncQuery> queryaddeler;
pthread_t *threads [MAX_THREAD_COUNT];
Job *jobs [MAX_THREAD_COUNT];
std::mutex Addmutex;
std::mutex Docsmutex;

void resetopt(bool (&opt)[6])
{
  for (int qwe=0;qwe<7;++qwe)
    opt[qwe]=false;
}

void printtable(bool (&opt)[6])
{
  for (int qwe=0;qwe<7;++qwe)
    cout<<opt[qwe]<<" ";
  cout<<endl;
}

void addopt(bool (&opt)[6],int i)
{
  if(i<3)
    {
      if (i == 2)
        {
          opt[5] = true;
          opt[2] = true;
        }
      else if (i == 1)
        {
          opt[5] = true;
          opt[4] = true;
          opt[2] = true;
          opt[1] = true;

        }
      else
        {
          opt[5] = true;
          opt[4] = true;
          opt[3] = true;
          opt[2] = true;
          opt[1] = true;
          opt[0] = true;
        }
    }
  else
    {
      if (i == 5)
        {
          opt[5] = true;
        }
      else if (i == 4)
        {
          opt[4] = true;
          opt[5] = true;
        }
      else
        {
          opt[5] = true;
          opt[4] = true;
          opt[3] = true;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
void * dowork(void *arg)
//void dowork(Job *j)
{
  Job *j = ((struct Job*)arg);

  unsigned int la=0,lb=0,dif=0;

  clock_t test = clock();
  char *savepointer;
  char * pch;
  unsigned int i,z,a,d;
  bool optimizer[6];
  deque<IncQuery>::iterator deqit;

  std::vector<QueryID> tempqueries;
  std::vector<Document> tempdoc;

  std::vector<SimplerQuery*> matched; // this needs .clear() every new document
  std::vector<SimplerQuery*>::iterator matchedit; // iterator for matched

  std::vector<QueryID>::iterator tempdocit;

  std::unordered_set<char*,c_Hash,c_cmp> words;
  std::unordered_set<char*,c_Hash,c_cmp>::iterator sit;

  std::unordered_set<char*,c_Hash,c_cmp> querywords;
  std::unordered_set<char*,c_Hash,c_cmp>::iterator qw;

  std::unordered_map<QueryID,int> qwordnumber;
  std::unordered_map<QueryID,int> tempqwordnumber;
  std::unordered_map<QueryID,int>::iterator qwordit;

  std::unordered_map<char*,set<SimplerQuery*>,c_Hash,c_cmp> Querysize[MAX_WORD_LENGTH+1];
  std::unordered_map<char*,std::set<SimplerQuery*>,c_Hash,c_cmp>::iterator qit;
  std::set<SimplerQuery*>::iterator it;
  SimplerQuery * sq;


  words.clear();
  resetopt(optimizer);

  //testtime(test,"Initialized");

  while(!j->finish)
    {

      if(!j->done)
        {
          //testtime(test,"Got Job");
          //----------------------MATCH DOCUMENT------------------------//
          if (j->jobtype == MATCH_DOC)
            {
              Addmutex.lock();
              if(!j->queryaddeler.empty())  a=j->queryaddeler.size();
              else a=0;
              Addmutex.unlock();

              //testtime(test,"Gotadelesizes");

              if(a>0)
                {
                  deqit=j->queryaddeler.begin();
                  int counter;
                  SimplerQuery *searchflag = new SimplerQuery;
                  for(i=0;i<a;++i)
                    {

                      if(deqit->jtype == ADD_QUERY)
                        {
                          pch = strtok_r(deqit->str," ",&savepointer);
                          while (pch != NULL)
                            {
                              ++counter;
                              SimplerQuery *query = new SimplerQuery;
                              *query = deqit->toSQuery();
                              strcpy(query->str,pch);
                              int size = strlen(pch);

                              qit = Querysize[size].find(pch);

                              if (qit != Querysize[size].end())
                                qit->second.insert(query);
                              else
                                {
                                  char *s = new char[MAX_WORD_LENGTH+1];
                                  strcpy(s,pch);
                                  Querysize[size][s].insert(query);
                                }
                              pch = strtok_r(NULL," ",&savepointer);
                            }
                          qwordnumber[deqit->query_id]=counter;
                          counter=0;
                          ++deqit;
                          continue;
                        }
                      else
                        {
                          for(z = 1 ;z < MAX_WORD_LENGTH+1; ++z)
                            for (qit = Querysize[z].begin();qit != Querysize[z].end();++qit)
                              {
                                *searchflag = deqit->toSQuery();
                                it = qit->second.find(searchflag);
                                if(it!= qit->second.end())
                                  {
                                    delete *it;
                                    qit->second.erase(it);
                                  }
                              }
                          qwordnumber.erase(deqit->query_id);
                          ++deqit;
                          continue;
                        }
                    }
                  j->queryaddeler.erase(j->queryaddeler.begin(),j->queryaddeler.begin()+a);
                  delete searchflag;
                }

              //testtime(test,"Adeletedqs ");

              tempqwordnumber = qwordnumber;

              //       ----  Document tokenization ----                //

              pch = strtok_r(j->doc.str," ",&savepointer);
              while (pch != NULL)
                {
                  if (words.find(pch)==words.end())
                    {
                      words.insert(pch);
                      lb = strlen(pch);
                      if(lb<=3)
                        {
                          i = 0;
                          z = lb+3;
                        }
                      else if(lb >=28)
                        {
                          i = lb-3;
                          z = 32;
                        }
                      else
                        {
                          i = lb-3;
                          z = lb+3;
                        }

                      qit = Querysize[lb].find(pch);
                      if (qit!= Querysize[lb].end())
                        {
                          for (it = qit->second.begin();it!=qit->second.end();++it)
                            {
                              if(!(*it)->matched)
                                {
                                  --tempqwordnumber[(*it)->query_id];
                                  (*it)->matched = true;
                                }
                            }
                        }
                      for(;i < z;++i)
                        {
                          for (qit = Querysize[i].begin();qit != Querysize[i].end();++qit)
                            {
                              la = i;
                              if (la>lb)
                                {
                                  dif = la-lb;
                                }
                              else if (lb>la)
                                {
                                  dif = lb-la;
                                }
                              else dif = 0;

                              resetopt(optimizer);

                              for (it = qit->second.begin();it!=qit->second.end();++it)
                                {
                                  sq = (*it);
                                  if(!sq->matched)
                                    {
                                      if (sq->match_type == MT_EDIT_DIST)
                                        {
                                          if(sq->match_dist >= dif)
                                            {
                                              if(optimizer[2+sq->match_dist])
                                                {
                                                  --tempqwordnumber[sq->query_id];
                                                  sq->matched = true;
                                                }
                                              else if (CheckEditDistance(sq->str,pch,sq->match_dist))
                                                {
                                                  --tempqwordnumber[sq->query_id];
                                                  sq->matched = true;
                                                  addopt(optimizer,2+sq->match_dist);
                                                }
                                            }
                                        }
                                      else if (sq->match_type == MT_HAMMING_DIST)
                                        {
                                          if(!dif)
                                            {
                                              if(optimizer[sq->match_dist-1])
                                                {
                                                  --tempqwordnumber[sq->query_id];
                                                  sq->matched = true;
                                                }
                                              else if (CheckHammingDistance(sq->str,pch,lb,sq->match_dist))
                                                {
                                                  --tempqwordnumber[sq->query_id];
                                                  sq->matched = true;
                                                  addopt(optimizer,sq->match_dist-1);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                  pch = strtok_r(NULL," ",&savepointer);
                }

              //testtime(test,"Tokenized  ");
              //       ----------------------------------              //


              for(qwordit = tempqwordnumber.begin();qwordit !=tempqwordnumber.end();++qwordit)
                {
                  if (qwordit->second==0)
                    {
                      tempqueries.push_back(qwordit->first);
                    }
                }

              std::sort(tempqueries.begin(),tempqueries.end());
              Document doc1 = j->doc.toDoc();
              doc1.num_res = tempqueries.size();
              if(doc1.num_res)
                doc1.query_ids=(unsigned int*)malloc(doc1.num_res*sizeof(unsigned int));
              else
                doc1.query_ids=(unsigned int*)malloc(sizeof(unsigned int));

              for(i=0;i<doc1.num_res;i++)
                doc1.query_ids[i]=tempqueries[i];

              Docsmutex.lock();
              docs.push_back(doc1);
              Docsmutex.unlock();

              for(z = 1 ;z < MAX_WORD_LENGTH+1; ++z)
                for (qit = Querysize[z].begin();qit != Querysize[z].end();++qit)
                  for (it = qit->second.begin();it!=qit->second.end();++it)
                    (*it)->matched = false;

              //                for(sit = words.begin();sit!=words.end();++sit)
              //                  delete *sit;

              delete[] j->doc.str;
              tempqueries.clear();
              tempqwordnumber.clear();
              words.clear();
            }
          //----------------------ADD QUERY--------------------//
          else  if (j->jobtype == ADD_QUERY)
            {
              for(i=0;i<MAX_THREAD_COUNT;++i)
                {
                  jobs[i]->queryaddeler.insert(jobs[i]->queryaddeler.end(),j->queryaddelerh.begin(),j->queryaddelerh.end());
                }
              Addmutex.unlock();
              //--------------------done-----------------------//
            }
          j->jobtype = NOTHING;
          j->done = true;
          //testtime(test,"Job Done");
        }
    }

  //cout<<"yielding"<<endl;
  j->exited = true;
  std::this_thread::yield();
}

ErrorCode InitializeIndex()
{
  Addmutex.unlock();
  Docsmutex.unlock();
  int i;
  pthread_t *threads1;
  threads1 = new pthread_t[MAX_WORD_LENGTH];// malloc(MAX_THREAD_COUNT * sizeof(*threads1));
  for(i=0;i<MAX_THREAD_COUNT;i++)
    {
      Job *j = new Job(i);
      jobs[i] = j;
      jobs[i]->finish = false;
      jobs[i]->done = true;
      pthread_create(&threads1[i],
          NULL,
          dowork,
          (void*)j);
      pthread_detach(threads1[i]);
    }

  //      threads[i] = thread(dowork,j);
  //      threads[i].detach();

  return EC_SUCCESS;
}



///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode DestroyIndex()
{
  bool done = false;
  do
    {
      for(int i=0;i<MAX_THREAD_COUNT;++i)
        if (jobs[i]->done)
          {
            jobs[i]->finish=true;
            done = true;
          }
      for(int i=0;i<MAX_THREAD_COUNT;++i)
        if (!jobs[i]->done)
          {
            done = false;
          }
      std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }while(!done);
  done = false;
  do
    {
      for(int i=0;i<MAX_THREAD_COUNT;++i)
        if (jobs[i]->exited==true)
          done = true;
      for(int i=0;i<MAX_THREAD_COUNT;++i)
        if (jobs[i]->exited==false)
          done = false;
    }while(!done);
  std::this_thread::sleep_for(std::chrono::milliseconds(0));
  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

int freethreadindex = 0;

ErrorCode StartQuery(QueryID query_id, const char* query_str, MatchType match_type, unsigned int match_dist)
{
  //  Query q;
  //  q.query_id=query_id;
  //  strcpy(q.str, query_str);
  //  q.match_type=match_type;
  //  q.match_dist=match_dist;
  //  adder.push_back(q);

  IncQuery q;
  q.query_id = query_id;
  strcpy(q.str,query_str);
  q.match_dist = match_dist;
  q.match_type = match_type;
  q.jtype = ADD_QUERY;
  queryaddeler.push_back(q);

  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode EndQuery(QueryID query_id)
{
  //  Query q;
  //  q.query_id = query_id;
  //  deleter.push_back(q);

  IncQuery q;
  q.query_id = query_id;
  q.jtype = DEL_QUERY;
  queryaddeler.push_back(q);

  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode MatchDocument(DocID doc_id, const char* doc_str)
{
  clock_t test = clock();
  bool submited = false;
  do
    {
      if(!queryaddeler.empty())
        {
          for(int i=freethreadindex;i<MAX_THREAD_COUNT;i++)
            if (jobs[i]->done)
              {
                jobs[i]->jobtype = ADD_QUERY;
                jobs[i]->queryaddelerh = queryaddeler;
                queryaddeler.clear();
                Addmutex.lock();
                jobs[i]->done = false;
                freethreadindex = ++i;
              }
          if (freethreadindex>=MAX_THREAD_COUNT-1)
            freethreadindex=0;
        }

      for(int i=freethreadindex;i<MAX_THREAD_COUNT;i++)
        {
          if (jobs[i]->done)
            {
              jobs[i]->doc.str = new char[MAX_DOC_LENGTH];
              jobs[i]->doc.doc_id = doc_id;
              strcpy(jobs[i]->doc.str, doc_str);
              jobs[i]->jobtype = MATCH_DOC;
              jobs[i]->done = false;
              submited = true;
              freethreadindex = ++i;
              break;
            }
        }
      if (freethreadindex>=MAX_THREAD_COUNT-1)
        freethreadindex=0;
      std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }while(!submited);
  //testtime(test,"Match Doc submitted in : ");

  return EC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode GetNextAvailRes(DocID* p_doc_id, unsigned int* p_num_res, QueryID** p_query_ids)
{
  // Get the first undeliverd resuilt from "docs" and return it
  *p_doc_id=0; *p_num_res=0; *p_query_ids=0;
  Docsmutex.lock();
  if(docs.empty())
    do
      {
        Docsmutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(0));
        Docsmutex.lock();

      }while(docs.empty());
  auto docid = docs[0].doc_id;
  auto resnum = docs[0].num_res;
  auto qids = docs[0].query_ids;
  *p_doc_id=docid; *p_num_res=resnum; *p_query_ids=qids;
  docs.erase(docs.begin());
  cout<<*p_doc_id<<" DONE"<<endl;
  Docsmutex.unlock();
  return EC_SUCCESS;
}
///////////////////////////////////////////////////////////////////////////////////////////////
//--------------------- USELESS CRAP------------------------------------////
//-----------------------done------------------------------//
//                              pch = strtok_r(j->doc.str," ",&savepointer);
//                              while (pch != NULL)
//                              {
////                                    char *str = new char(MAX_WORD_LENGTH);
////                                    strcpy(str,pch);
////                                    if(find(words.begin(),words.end(),str)==words.end())
////                                    words.insert(str);
//                                      words.insert(pch);
//                                      pch = strtok_r(NULL," ",&savepointer);
//                              }
//
//                              cout<<"DOC SIZE IS "<<words.size()<<endl;

//              tempqwords = qwords;
//              int larger = 0;
//              for (sit = words.begin();sit!=words.end();++sit)
//              {
//                      for (qit = Querywords.begin();qit != Querywords.end();++qit)
//                      {
//
//                              la = strlen(qit->first);
//                              lb = strlen(*sit);
//                              if (la>lb)
//                              {
//                                      if (la-lb>3)
//                                              continue;
//                                      dif = la-lb;
//                                      larger=la;
//
//                              }
//                              else if (lb>la)
//                              {
//                                      if (lb-la>3)
//                                              continue;
//                                      dif = lb-la;
//                                      larger=lb;
//                              }
//                              else
//                                      larger = la;
//
//                              if (memcmp(*sit,qit->first,larger)==0)
//                              //if (strcmp(*sit,qit->first)==0)
//                              {
//                                      for (it = qit->second.begin();it!=qit->second.end();++it)
//                                              {
//                                                      if(find(j->ignore.begin(),j->ignore.end(),it->query_id)==j->ignore.end())
//                                                      {
//                                                              --tempqwords[it->query_id];
//                                                              querieetouched[it->query_id] = true;
//                                                      }
//                                              }
//                              }
//                              curquery = qit->second;
//                              for (it = curquery.begin();it<curquery.end();++it)
//                              {
//
//                                      if (it->match_type == MT_EDIT_DIST)
//                                      {
//                                              if (CheckEditDistance(it->str,la,*sit,lb,it->match_dist))
//                                              {
////                                                                    if(it->query_id==96)
////                                                                            cout<<"QUERY 7 edit "<<it->str<<" = "<<*sit<<endl;
//                                                      //cout<<it->str<<" = "<<*sit<< " for edit distance "<<it->match_dist<<" query: "<<it->query_id<<endl;
//                                                      --tempqwords[it->query_id];
//                                                      querieetouched[it->query_id] = true;
//                                              }
//                                      }
//                                      else if (it->match_type == MT_HAMMING_DIST)
//                                      {
//                                              if (CheckHammingDistance(it->str,la,*sit,lb,it->match_dist))
//                                              {
//                                                      if(j->doc.doc_id==1 && it->query_id==80)
//                                                              cout<<"YEEEEEEEEEEY              YEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEY"<<endl;
////                                                                    if(it->query_id==96)
////                                                                            cout<<"QUERY 7 hamming "<<it->str<<" = "<<*sit<<endl;
//                                                      //cout<<it->str<<" = "<<*sit<< " for hamming distance "<<it->match_dist<<" query: "<<it->query_id<<endl;
//                                                      --tempqwords[it->query_id];
//                                                      querieetouched[it->query_id] = true;
//                                              }
//                                      }
////                                                    else
////                                                    {
////                                                            if (memcmp(it->str,sit->c_str(),larger)==0)
////                                                            {
////                                                                    if(it->query_id==7)
////                                                                            cout<<"QUERY 7 memcmp "<<it->str<<" = "<<*sit<<endl;
////                                                                    //cout<<it->str<<" = "<<*sit<< " for exact match"<<" query:  "<<it->query_id<<endl;
////                                                                    tempqwords[it->query_id].push_back(true);
////                                                                    tempqwords[it->query_id].erase(tempqwords[it->query_id].begin());
////                                                                    querieetouched[it->query_id] = true;
////
////                                                            }
////                                                    }
//
//
//                              }
//                      }
//              }
//              for(quertit = querieetouched.begin();quertit != querieetouched.end();++quertit)
//              {
//                      if (quertit->second)
//                      {
//                              QueryID t = quertit->first;
//                              if (tempqwords[t]==0)
//                              {
//                                      tempqueries.push_back(quertit->first);
//                              }
//                      }
//              }
//              Docsmutex.lock();
//              std::sort(tempqueries.begin(),tempqueries.end());
//              Document doc1 = j->doc.toDoc();
//              doc1.num_res = tempqueries.size();
//              if(doc1.num_res)
//                      doc1.query_ids=(unsigned int*)malloc(doc1.num_res*sizeof(unsigned int));
//              for(i=0;i<doc1.num_res;i++)
//                      doc1.query_ids[i]=tempqueries[i];
//              docs.push_back(doc1);
//              Docsmutex.unlock();
//              tempqueries.clear();
//      }
//
//      j->doc.str = NULL;
//      j->jobtype = NOTHING;
//      j->ignore.clear();
//      words.clear();
//      querieetouched.clear();
//      tempqwords.clear();



////////////---------------------------------------------------------------////////////////////////


//                              pch = strtok_r(j->query.str," ",&savepointer);
//                              while (pch != NULL)
//                              {
//                                      //char* str = new char(MAX_QUERY_LENGTH);
//                                      //strcpy(str, pch);
//                                      //querywords.insert(str);
//                                      querywords.insert(pch);
//                                      pch = strtok_r(NULL," ",&savepointer);
//                              }
//                              Querystringsmutex.lock();
//                              tempfound=0;
//                              for(qw = querywords.begin();qw!=querywords.end();++qw)
//                              {
//                                      Query *newq = new Query;
//                                      newq->match_dist = j->query.match_dist;
//                                      newq->match_type = j->query.match_type;
//                                      newq->query_id = j->query.query_id;
//                                      strcpy(newq->str,*qw);
//                                      tempfound++;
//                                      it = std::find(Querystrings[newq->str].begin(),Querystrings[newq->str].end(),*newq);
//                                      Querystrings[newq->str].erase(it);
//                                      auto qwit = std::find(qwords[newq->query_id].begin(),qwords[newq->query_id].end(),newq->str);
//                                      qwords[newq->query_id].erase(qwit);
//                              }
//                              Querystringsmutex.unlock();
//                              qwords.erase(j->query.query_id);
//                              tempfound=0;


////-----------------------------------------------------------------------------//////////////





//                              pch = strtok_r(j->query.str," ",&savepointer);
//                              querywords.clear();
//                              while (pch != NULL)
//                              {
////                                    char* str = new char(MAX_QUERY_LENGTH);
////                                    strcpy(str, pch);
////                                    querywords.insert(str);
//                                      querywords.insert(pch);
//                                      pch = strtok_r(NULL," ",&savepointer);
//                              }
//                              Querystringsmutex.lock();
//                              for(qw = querywords.begin();qw!=querywords.end();++qw)
//                              {
//                                      Query *newq = new Query;
//                                      newq->match_dist = j->query.match_dist;
//                                      newq->match_type = j->query.match_type;
//                                      newq->query_id = j->query.query_id;
//                                      strcpy(newq->str,*qw);
//                                      qwords[newq->query_id].push_back(newq->str);
//                                      Querystrings[newq->str].push_back(*newq);
//                              }
//                              Querystringsmutex.unlock();
//                              querywords.clear();
//                              //qwords[j->query.query_id] = tempfound;

/////--------------------------------------------------------------------------------/////////////////////////


//      char cur_doc_str[MAX_DOC_LENGTH];
//      strcpy(cur_doc_str, doc_str);
//      unsigned int i, n=queries.size();
//      vector<unsigned int> query_ids;
//      // Iterate on all active queries to compare them with this new document
//      for(i=0;i<n;i++)
//      {
//              bool matching_query=true;
//              Query* quer=&queries[i];
//
//              int iq=0;
//              while(quer->str[iq] && matching_query)
//              {
//                      while(quer->str[iq]==' ') iq++;
//                      if(!quer->str[iq]) break;
//                      char* qword=&quer->str[iq];
//
//                      int lq=iq;
//                      while(quer->str[iq] && quer->str[iq]!=' ') iq++;
//                      char qt=quer->str[iq];
//                      quer->str[iq]=0;
//                      lq=iq-lq;
//
//                      bool matching_word=false;
//
//                      int id=0;
//                      while(cur_doc_str[id] && !matching_word)
//                      {
//                              while(cur_doc_str[id]==' ') id++;
//                              if(!cur_doc_str[id]) break;
//                              char* dword=&cur_doc_str[id];
//
//                              int ld=id;
//                              while(cur_doc_str[id] && cur_doc_str[id]!=' ') id++;
//                              char dt=cur_doc_str[id];
//                              cur_doc_str[id]=0;
//
//                              ld=id-ld;
//
//                              if(quer->match_type==MT_EXACT_MATCH)
//                              {
//                                      if(strcmp(qword, dword)==0) matching_word=true;
//                              }
//                              else if(quer->match_type==MT_HAMMING_DIST)
//                              {
//                                      //unsigned int num_mismatches=HammingDistance(qword, lq, dword, ld);
//                                      //if(num_mismatches<=quer->match_dist) matching_word=true;
//                                      if(HammingDistance(qword, lq, dword, ld, quer->match_dist)) matching_word=true;
//                              }
//                              else if(quer->match_type==MT_EDIT_DIST)
//                              {
//                                      //unsigned int edit_dist=EditDistance(qword, lq, dword, ld);
//                                      //if(edit_dist<=quer->match_dist) matching_word=true;
//                                      if(CheckEditDistance(qword, lq, dword, ld,quer->match_dist)) matching_word=true;
//                              }
//
//                              cur_doc_str[id]=dt;
//                      }
//
//                      quer->str[iq]=qt;
//
//                      if(!matching_word)
//                      {
//                              // This query has a word that does not match any word in the document
//                              matching_query=false;
//                      }
//              }
//
//              if(matching_query)
//              {
//                      // This query matches the document
//                      query_ids.push_back(quer->query_id);
//              }
//      }
//
//      Document doc;
//      doc.doc_id=doc_id;
//      doc.num_res=query_ids.size();
//      doc.query_ids=0;
//      if(doc.num_res) doc.query_ids=(unsigned int*)malloc(doc.num_res*sizeof(unsigned int));
//      for(i=0;i<doc.num_res;i++) doc.query_ids[i]=query_ids[i];
//      // Add this result to the set of undelivered results
//      docs.push_back(doc);
//////////////////////////////////////////////////////////////////////////////////////////////////////
//int EditDistance(char* a, int na, char* b, int nb)
//{
//      unsigned int oo=0x7FFFFFFF;
//      static int T[2][MAX_WORD_LENGTH+1];
//      int ia, ib;
//      int cur=0;
//      ia=0;
//      for(ib=0;ib<=nb;ib++)
//              T[cur][ib]=ib;
//      cur=1-cur;
//      for(ia=1;ia<=na;ia++)
//      {
//              for(ib=0;ib<=nb;ib++)
//                      T[cur][ib]=oo;
//
//              int ib_st=0;
//              int ib_en=nb;
//
//              if(ib_st==0)
//              {
//                      ib=0;
//                      T[cur][ib]=ia;
//                      ib_st++;
//              }
//
//              for(ib=ib_st;ib<=ib_en;ib++)
//              {
//                      int ret=oo;
//
//                      int d1=T[1-cur][ib]+1;
//                      int d2=T[cur][ib-1]+1;
//                      int d3=T[1-cur][ib-1]; if(a[ia-1]!=b[ib-1]) d3++;
//
//                      if(d1<ret) ret=d1;
//                      if(d2<ret) ret=d2;
//                      if(d3<ret) ret=d3;
//
//                      T[cur][ib]=ret;
//              }
//
//              cur=1-cur;
//      }
//
//      int ret=T[1-cur][nb];
//
//      //if(ret<4)
////    if(strcmp(a,"airlines")==0 || strcmp(a,"airport")==0)
////      {
////        cout<<"ALALALALALLALALALAL "<<a<<" "<<na<<" "<<b<<" "<<nb<<endl;
////          cout<<"222222 "<<ret<<endl;
////      }
//      return ret;
//}

