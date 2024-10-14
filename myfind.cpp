#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cerrno>
#include <algorithm> 
#include <filesystem>   // C++17 file system operations
#include <unistd.h>  // For POSIX functions
#include <sys/file.h>   // file locking 
#include <sys/wait.h>   // handling child processes
#include <sys/types.h>  // data types

using namespace std;

// global flags for case sensitivity and recursion
bool case_sensitive = true;
bool recursive = false;

void search_file (const filesystem::path &search_path, const string file_name, pid_t pid, int file_descriptor) // function to search files in directory
{
   try
   {
      for (const auto& entry : filesystem::directory_iterator(search_path))   // iterate entries in directory
      {
         if (entry.is_directory())
         {
            if (recursive) // search recursive if enabled
            {
               search_file(entry.path(), file_name, pid, file_descriptor); // recursive call for subdirectories
            }
         } else
         {
            string current_file = entry.path().filename().string();  // extract filename from path

            bool match = !case_sensitive  // perform case sensitive or insensitive search
            ?  equal(current_file.begin(), current_file.end(), file_name.begin(), file_name.end(), [](char a, char b)
            {
               return tolower(a) == tolower(b); // compare ignoring case
            })
            : current_file == file_name;  // compare exact case

            if (match)
            {
               string full_path = filesystem::absolute(entry.path()).string();   // return path if math is found
               // synchronize with flock to avoid race conditions
               flock(file_descriptor, LOCK_EX);
               cout << pid << ": " << file_name << ": " << full_path << endl;
               cout.flush();
               flock(file_descriptor, LOCK_UN);
            }
         }
      }
   } 
   catch (const filesystem::filesystem_error &error)   // catch filesystem-related errors
   {
      flock (file_descriptor, LOCK_EX);   // synchronize error output
      cerr << "Error opening directory" << search_path << ": " << error.what() << endl;
      flock (file_descriptor, LOCK_UN);   // unlock stdout
   }
}

int main(int argc, char *argv[])
{
   int option; // variable for option parsing
   string search_path;  // variable for search path
   vector<string> file_names; // vector to store searched filenames

   if (argc < 3)  // check if enough arguments are provided
   {
      cerr << "Arguments needed: " << argv[0] << " [-R] [-i] searchpath filenames." << endl;
      return EXIT_FAILURE;
   }

   while ((option = getopt(argc, argv, "Ri")) != -1)  // option parsing using getopt()
   {
      switch(option)
      {
         case 'R':   // enable recursive search
         recursive = true; 
         break;
         
         case 'i':   // enable case insensitive search
         case_sensitive = false;
         break;

         default: // return information for invalid inputs
         cerr << "Arguments needed: " << argv[0] << " [-R] [-i] searchpath filenames." << endl;
         return EXIT_FAILURE;
      }
   }

   if (optind >= argc)  // check if searchpath is provided
   {
      cerr << "Searchpath and filenames expected after options." << endl;
      return EXIT_FAILURE;
   }

   search_path = argv[optind];   // assign first argument after options as searchpath
   optind++;   // move to next argument for filenames

   for (int i = optind; i < argc; i++) // collect all filenames
   {
      file_names.emplace_back(argv[i]);   // add filenames to vector
   }
   
   int file_descriptor = fileno(stdout);   // get file descriptor for stdout to lock input

   for (const auto &file_name : file_names)  // loop and fork a process for each filename
   {
      pid_t pid = fork();  // fork new process

      if (pid < 0)   // print error if fork fails
      {
         perror("Failed to fork process.");
         return EXIT_FAILURE;
      }

      if (pid == 0)  // child process searches for current filename
      {
         search_file(search_path, file_name, getpid(), file_descriptor);
         exit(0); // exit child process after search is complete
      }
   }

   int status; // parent process waits for all child processes to prevent zombies
   while (wait(&status) > 0);

   return 0;
}