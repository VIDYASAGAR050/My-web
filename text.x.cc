#include<mutex>
#include<condition_variable>
#include<thread>
#include<iostream>
#include<fstream>
#include<future>
#include<string>
class ReadersWriters{
    private:
        std::mutex mtx;
        std::condition_variable cv;
        bool is_writing;
        int readers;
    public:
        ReadersWriters():is_writing(false),readers(0){}
        void startRead();
        void endRead();
        void startWrite();
        void endWrite();
};

void ReadersWriters::startRead()
{
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]()
            { return !is_writing; });
    ++readers;
}
void ReadersWriters::endRead()
{
    std::unique_lock<std::mutex> lock(mtx);
    if( --readers ==0 && is_writing)
    {
        cv.notify_all();
    }
}
void ReadersWriters::startWrite()
{
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]()
            { return !is_writing && readers == 0; });
    is_writing = true;
}
void ReadersWriters::endWrite()
{
    std::unique_lock<std::mutex> lock(mtx);
    is_writing = false;
    cv.notify_all();
}

class File{
    private:
        std::fstream filename;
        ReadersWriters Lock;
    public:
        File(const std::string& filename__): filename(filename__){
            if(!filename.is_open())
            {
                filename.open(filename__, std::ios::out);
            }
            filename.close();
            filename.open(filename__, std::ios::out | std::ios::in);
            // create test.txt
            //  std::ofstream outfile("test.txt");
            //  filename.open(filename__, std::ios::in | std::ios::out | std::ios::app);
        }
        std::string Read()
        {
            Lock.startRead();
            filename.clear();
            filename.seekg(0, std::ios::beg);
            std::string line;
            std::string content="";
            while (std::getline(filename, line))
            {
                content += line + "\n";
            }
            Lock.endRead();
            return content;
        }
        void Write(const std::string &data)
        {
            Lock.startWrite();
            filename.clear();
            filename.seekp(0, std::ios::end);
            filename << data;
            Lock.endWrite();
        }
};
int main()
{
    File file("test.txt");

    std::promise<void> writeFinished;
    std::future<void> writeFinishedFuture = writeFinished.get_future();
    std::thread t1([&file]()
                   {
        std::cout << "Thread 1 has started to write" << std::endl;
        file.Write("Thread 1 has written\n");
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
        std::cout << "Thread 1 has finished writing" << std::endl; });
    std::thread t2([&file, &writeFinishedFuture]()
                   { writeFinishedFuture.get();
                    std::cout << "Thread 2 has started to read\n";
                    std::cout << "(line)Thread 2: " <<file.Read() << std::endl; 
                    std::this_thread::sleep_for(std::chrono::seconds(1)); 
                    std::cout << "Thread 2 has finished reading" << std::endl; });
    std::thread t3([&file, &writeFinished]()
                   { std::cout << "Thread 3 has started to write" << std::endl;
                   file.Write("This is from thread 3");
                   std::this_thread::sleep_for(std::chrono::seconds(1));
                   std::cout << "Thread 3 has finished writing" << std::endl;
                   writeFinished.set_value();
                 });
    t1.join();
    t2.join();
    t3.join();
    return EXIT_SUCCESS;
}