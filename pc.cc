#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<vector>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

//change following to see it break
//define NO_SYNCRO

using std::vector;

const int MAX_prod=1000;

const int buffer_size = 4;

/*
 * This struct is for all the shared data, including the mutex
 * For the producer-consumer/bounded buffer problem all threads access the SAME
 * Shared data.
 *
 * If you need both shared and thread-unique data like an index or id,
 * define another struct that has a pointer to a shared, allocate one instance of the shared,
 * and then create & initialize each struct for each thread.
 */
typedef struct __shared {
    int ct;
    
    sem_t mutex;
    sem_t full;
    sem_t empty;
    int in;
    int out;
    std::vector<int> ns;
    int buffer[buffer_size];

} shared;

void* producer_thread(void* arg){
    shared* sh = (shared*) arg;

    bool more_work = true;
    while(more_work){
        usleep(100); //this is a good way to do "non-CPU" fake work, just wait. # is in MICROsecondss.
        #ifndef NO_SYNCRO
        sem_wait(&(sh->empty));
        sem_wait(&(sh->mutex));
        #endif

        if(sh->ns.size()>=MAX_prod){
            more_work = false; 
            #ifndef NO_SYNCRO
            //shameless hack
            sem_post(&(sh->mutex));
            sem_post(&(sh->empty));
            sem_post(&(sh->full));
            #endif
            break;
            
        } else{

            int n = sh->ct;
            ++(sh->ct);
            sh->buffer[sh->in] = n;
            sh->in = (sh->in +1)%buffer_size;
        }

        #ifndef NO_SYNCRO
        sem_post(&(sh->mutex));
        sem_post(&(sh->full));
        #endif

    }
    std::cout<<"producer ending"<<std::endl;
    bool* ok = new bool;
    *ok = 1;
    return ok;
}

void* consumer_thread(void* arg){
    shared* sh = (shared*) arg;

    bool more_work = true;
    while(more_work){
         std::cout<<"x"<<std::endl;
        usleep (100);
        #ifndef NO_SYNCRO
        sem_wait(&(sh->full));
        sem_wait(&(sh->mutex));
        #endif

        if(sh->ns.size()>=MAX_prod ) {
            more_work = false;
            #ifndef NO_SYNCRO
            //shameless hack
            sem_post(&(sh->mutex));
            sem_post(&(sh->full));
            sem_post(&(sh->full));
            #endif
            break;
        } else {
            std::cout<<"ns.size() == "<<sh->ns.size()<<std::endl;
            sh->ns.push_back(sh->buffer[sh->out]);
            sh->out = (sh->out +1)%buffer_size;
        }

        #ifndef NO_SYNCRO
        sem_post(&(sh->mutex));
        sem_post(&(sh->empty));
        #endif
    }
    std::cout<<"consumer ending"<<std::endl;

    bool* ok = new bool;
    *ok = true;
    return ok;
    
}

int pc(const int num_p, const int num_c){
    // using pthread;


    //semaphore and shared initialization code
    shared sh;
    sem_init(&sh.empty,0,buffer_size);
    sem_init(&sh.full,0,0);
    sem_init(&sh.mutex,0,1);
    
    sh.in = 0;
    sh.out = 0;

    sh.ct = 0;

    /*
     * pthread_create() 1st argument: the thread to initialize (in ps/cs)
     *   2nd argument: leave as NULL often
     *   3rd argument: the function to call for the thread
     *   4th argument: the SINGLE argument to the thread function (3rd argument)
     *           If you want to pass multiple argments to the thread arg (called arg, above), use a struct
     *           If each thread gets DIFFERENT arguments, instantiate a new struct for each (not done here)
     *
     *   Below code has two loops for different types of threads (consumers and producers)
     *      -> code with one type would have one loop
     *
     *   The ps and cs arrays are useful here because of the pthread_join code,
     *       which waits until are done. 
     */
    pthread_t ps[num_p];
    for(int i =0;i<num_p;++i){
        pthread_create(&ps[i],NULL,producer_thread,&sh);
    }
    pthread_t cs[num_c];
    for(int i =0;i<num_c;++i){
        pthread_create(&cs[i],NULL,consumer_thread,&sh);
    }


    bool* ret;
    bool all_okay = true;
    for(int i =0;i<num_p;++i){
        pthread_join(ps[i],(void**) &ret);
        all_okay &= *ret;
        delete ret;

    }

    for(int i =0;i<num_c;++i){
        pthread_join(cs[i],(void**) &ret);
        all_okay &=*ret;
        delete ret;
    }

    std::cout<<"all_okay, returns: "<<all_okay<<std::endl;

    // pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; // if you don't want to init a semaphore

    using std::cout;
    using std::endl;

    int prev = -1;
    bool prev_ok = true;
    for(int n: sh.ns){
        cout<<n<<" ";
        if (n-prev != 1){
            prev_ok = false;
        }
        prev = n;
    }
    cout<<"\nAll okay? "<<prev_ok;

    return !prev_ok;//traditionally, 0 okay for exit code
}

int main(){
    using std::cout;
    using std::endl;
    cout<<"Hello world"<<endl;
    
    int ok = 0;
    for(int i = 0;i<100 && !ok; ++i ){
        ok=pc(2,4);//pc() was originally a main(). This code is just testing the implementation
        cout<<"Round "<<i<<endl;
        // int dump;
        // std::cin>>dump;
    }
    std::cout<<"\nfinished with ok ==" << ok <<" (0 is good)";

}
