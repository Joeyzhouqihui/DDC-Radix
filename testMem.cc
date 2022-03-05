#include <iostream>
#include <string>
#include <sys/time.h>
#include <libmemcached/memcached.h>

using namespace std;

int main(int argc,char *argv[]) {
  //connect server
  memcached_st *memc;
  memcached_return rc;
         memcached_server_st *servers;
         time_t expiration = 180;
         uint32_t flags = 0;
         memc = memcached_create(NULL);
         servers = memcached_server_list_append(NULL, "localhost", 11211, &rc);
         rc = memcached_server_push(memc, servers);

         if (rc != MEMCACHED_SUCCESS) {
           cout<<"connect fail"<<endl;
         }
         cout<<rc<<" "<<MEMCACHED_SUCCESS<<endl;

         memcached_server_list_free(servers);
         string key = "key";
         string value = "0";
         size_t value_length = value.length();
         size_t key_length = key.length();

         struct timeval start;
         struct timeval end;
         gettimeofday(&start, NULL);

          const char *SERVER_NUM_KEY = "serverNum";
          uint64_t serverNum;

          rc = memcached_set(memc, SERVER_NUM_KEY, strlen(SERVER_NUM_KEY), 
                              value.c_str(), value_length, expiration, flags);
         for(int i=0; i < 5; ++i)
         {
           rc = memcached_increment(memc, SERVER_NUM_KEY, strlen(SERVER_NUM_KEY), 1,
                             &serverNum);
            if (rc != MEMCACHED_SUCCESS) {
              cout<<"fail"<<endl;
            }
            cout<<"server number: "<<serverNum<<endl;
        //     //Save data
        //     rc = memcached_set(memc, key.c_str(), key.length(), value.c_str(), value.length(), expiration, flags);
        //     if(rc == MEMCACHED_SUCCESS)
        //     {
        //             cout<<"Save data: "<<value<<" successful!"<<endl;
        //     }
        //     //Get data
        //     char* result = memcached_get(memc, key.c_str(), key_length, &value_length, &flags, &rc);
        //     if(rc == MEMCACHED_SUCCESS)
        //     {
        //             cout<<"Get value: "<<value<<"by key "<<key<<"successful!"<<endl;
        //     }
        // //Delete data
        // rc = memcached_delete(memc, key.c_str(), key_length, expiration);
        // if(rc == MEMCACHED_SUCCESS)
        // {
        //             cout<<"Delete data: "<<value<<" successful!"<<endl;
        // }

         }
         gettimeofday(&end, NULL);
         cout<<"Use Time"<<start.tv_sec-end.tv_sec<<"sec, "<<start.tv_usec-end.tv_usec<<"usec";

      //free
      memcached_free(memc);
      return 0;
}