#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h> 
#include <ctype.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_PATH 500 // 500 chars per path
#define NAME 128  // size of process name
#define PROC  50 // size of process array
#define INPUT_MAX 128 // maximum input length
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

typedef struct process_info {  // a struct to hold the running and finished info
    char name[NAME];  // holds process name
    pid_t pid;  // holds process id
    int pindx;  // holds process index
} procInfo;  // name of the type
typedef struct bookmark_info {
    char name[NAME];
    int indx;
    int exists;
} bookmark;

pid_t currentlyRunningProc = 0; // will hold the currently runnnig process, initially none (0)
bookmark bookmarked[PROC]; // holds the bookmarked processes' informations
procInfo runningProc[PROC];  // holds the running processes' informations
procInfo finishedProc[PROC];  // holds the finished proceses' informations
bookmark CopiedBookmark[PROC]; // holds copied information of bookmarked

void setup(char inputBuffer[], char *args[],int *background){
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);


} /* end of setup routine */

int toFile(char *fileName, char *job) { // used for I/O operations
    int file;  // will contain the new file descriptor
    if(strcmp(job, "trunc") == 0){  // if the user enters > to truncate file
        file = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);  // a new file is opened, created if does not exist, truncates
    }else if (strcmp(job, "append") == 0) {  // if the user enters >> to append to the file
        file = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0777); // "                        ^^                    " appends
    }
    if(file == -1){  // if a an error occurs
        fprintf(stderr, "\nAn error occured while opening %s\n\n", fileName);
        return -1;  // terminates function and returns a response to the caller
    }else {  // if no errors occur
        int file2 = dup2(file, STDOUT_FILENO); // the file descriptor is duplicated to stdout's descriptor
        close(file);  // original file is closed
    }
    return 1;
    
}

int inputFromFile(char*fileName) {
    int file;
    file = open(fileName, O_RDONLY); // file is opened when user enters < to read input
    if(file == -1) {  // prints out an error and terminates function
        fprintf(stderr, "\nAn error occured while trying to read %s\n\n", fileName);
        return -1;
    }else{  // similar to toFile but it replaces the std input rather than the stdout
        int file2 = dup2(file, STDIN_FILENO);
        close(file); //closes original 
    }
    return 1;  // returns a positive feedback telling the caller the function has finsihed wit no errors
}

int errorToFile(char *fileName) {
    int file;
    file = open(fileName, O_WRONLY | O_CREAT, 0777); // opens a files, creates it, truncates it with permissions 777
    if(file == -1){  // an error occuured while opening the file
        fprintf(stderr, "\nAn error occured while opening %s\n\n", fileName);
        return -1;
    }else {
        int file2 = dup2(file, 2);
        close(file);
    }
    return 1;
}

int inputOutputFile(char *inputFile, char *outputFile) {
    int input, output;
    input = open(inputFile, O_RDONLY); // reads the given input files
    if(input == -1) {
        fprintf(stderr, "\nAn error while trying to read %s\n\n", inputFile);
        return -1;
    }else{
        int input2 = dup2(input, STDIN_FILENO);
        close(input);
    }
    output = open(outputFile, O_WRONLY | O_CREAT, 0777); // writes or/and creates the given output file
    if(output == -1) {
        fprintf(stderr, "\nAn error has occured while opening %s\n\n", outputFile);
        return -1;
    }else {
        int output2 = dup2(output, STDOUT_FILENO);
        close(output);
    }
    return 1;
}




char checkArrsEmpty(int bothArrs) { // checks if the arrays are empty
    int i;
    for(i=0; i<PROC; i++) {
        if(runningProc[i].pid != 0) { // if a process exists in the running array
            return 'F';  // return false
        }
        if(finishedProc[i].pid != 0 && bothArrs) { // if a process exists in the finished array
            return 'F';  // return false
        }
    }
    return 'T';  // return true
}

int findCommandPath(char* command, char* dirStart, char* foundPath) {
    DIR *dir = opendir(dirStart); // opens the given directory
    struct dirent *fileDir;
    char path[MAX_PATH]; // used to store the path
    int pathIsFound = 0; // used to tell if the path was found or not
    while(dir != NULL && (fileDir = readdir(dir))!= NULL) { // reads the opened directory and loops until through it
        if((strcmp(fileDir->d_name, ".") == 0 ) || (strcmp(fileDir->d_name, "..") == 0) || (strcmp(fileDir->d_name, "X11") == 0)) {
            continue;
        } // skips the files and folders that are either . or .. or X11
        strcpy(path, ""); // clears path
        strcat(path, dirStart);  // copies the directory's path to path ex. dir=/home then path=/home 
        if(strlen(path)>1) { // if dir is root / don't copy '/' to path
            strcat(path, "/"); // add '/' to path. ex. if path=/home, path now = /home/
        }
        strcat(path, fileDir->d_name); // copies the file or the folder name to path. ex. folder=basil then path=/home/basil
        if(strcmp(fileDir->d_name, command) == 0) { // if the file equals to the given command
            strcpy(foundPath, path); // copy the name of the file to path
            pathIsFound = 1; // sets pathIsFound to true
            break; // breaks the loop

        }
        DIR *subDir = opendir(path); // opens the current path
        if((subDir != NULL)) {  // if the current path is a directory
            findCommandPath(command, path, path);  // recursivly go into the sub direcotry ans its sub direcotries
        }
        closedir(subDir); // closes the sub direcotry

    }
    closedir(dir); // closes the direcotory
    if(!pathIsFound) {
        strcpy(path, "PATHNOTFOUND");
    }
}


void addProc(char* name, pid_t pid, procInfo *arr, int *processIndex) {
    if(checkArrsEmpty(1) == 'T') {
        *processIndex = 1;
    } else {
        *processIndex = *processIndex + 1;
    }
    procInfo newProc;  // creates new process
    newProc.pid = pid;  // assigns its pid to the given pid
    strcpy(newProc.name, name); // assigns its name to the given name
    newProc.pindx = *processIndex; // assigns the new process' index to the appropraite index
    arr[*processIndex] = newProc; // adds the process to the given array
}


void updateRunFinArr(pid_t finishedChild) {
    int i;
    for(i=0; i<PROC; i++) { // removes process from running array and puts it in the finish array
            if(finishedChild == runningProc[i].pid) {
                finishedProc[i].pid = finishedChild;  // copies running process' pid to finish's pid
                finishedProc[i].pindx = i;  // copies running process index to finish's index
                strcpy(finishedProc[i].name, runningProc[i].name);  // copies name of running process to finish process
                runningProc[i].pid = 0; // resets the process' information in the array
            }
        }
}

void checkOnChildren() {
    int i;
    pid_t finishedChild;
    finishedChild = waitpid(-1, NULL, WNOHANG); // checks if anychild finished
    if(finishedChild == -1) {
    }  // if no child finished, do nothing and just continue
    while(finishedChild != 0) {
        if(finishedChild == -1) {
            break;  // if no more children finsihed break the loop and contiue
        }
        updateRunFinArr(finishedChild);  // updates the running and finsihed lists appropriatly
        finishedChild = waitpid(-1, NULL, WNOHANG); // checks on the next child
    }
}


void ps_all() {
    int i;
    if(checkArrsEmpty(1) == 'F') {
        printf("\nRunning:\n");
        for(i=0; i<PROC; i++) {  // loops through the stored running processes' information
            if(runningProc[i].pid != 0) {
                printf("[%d]\t%s (Pid=%ld)\n", runningProc[i].pindx, runningProc[i].name, (long)runningProc[i].pid);
            }
        }
        printf("Finished:\n");
        for(i=0; i<PROC; i++) {
            if(finishedProc[i].pid != 0) { // loops through the stored running processes' information
                printf("[%d]\t%s \n", finishedProc[i].pindx, finishedProc[i].name);
                finishedProc[i].pid = 0;
            }
        }
        printf("\n"); // prints an empty line
    }   
    else {
        printf("\nThere are currenlty no running nor finished processes in the background\n\n");
    }
}



void findCommandEnv(char* foundPath, char* command, char* pathEnv) {
    char tempPathEnv[MAX_PATH]; // holds paths in the PATH variable temp
    char *eachPath;  // used to point to each path
    strcpy(tempPathEnv, pathEnv);  // copies the given PATH variable to tempPath
    eachPath = strtok(tempPathEnv, ":");  // splits strings in each ':' occurance
    while(eachPath != NULL) {  // loops until there are no more paths in the PATH variable
        findCommandPath(command, eachPath, foundPath); // finds the path of the command
        if(strlen(foundPath)>0) { // once a path is found break from the loop
            break;
        }
        eachPath = strtok(NULL, ":");
    }

}

void createProccess(char* commandPath, char *args[],  const int* background, 
    char *command, int *processIndex, char job[], char file[],
    char file2[], char *fullCommand) {
    pid_t id; // stores the child pid for the parent
    char commandNotFound[MAX_PATH]; // used to tell the parent if the command was found or not
    currentlyRunningProc = 0;
    id = fork();  // creates a new process (child)
    if(id==-1){  // if error occurs during forking
        fprintf(stderr, "\nAn error has occured while forking\n\n"); // prints error to stderr                              
    }   
    if(!id) {  // if statement only child can enter
        if ((strcmp(job, "trunc") == 0 || strcmp(job, "append") == 0) && 
            strcmp(file, "None") != 0 ){ // if user selected > or >>
            if(toFile(file, job) == -1){
                exit(1);
            }
        }
        else if (strcmp(job, "input") == 0 && strcmp(file, "None") != 0){
            if(inputFromFile(file) == -1){ // if user selected <
                exit(1);

            }
        }
        else if (strcmp(job, "error") == 0 && strcmp(file, "None") != 0) {
            if(errorToFile(file) == -1) {  // if user selected 2>
                exit(1);
            }
        }
        else if (strcmp(job, "inputWrite") == 0 && strcmp(file, "None") != 0 && 
            strcmp(file2, "None") != 0) { // user selected < input > output
            if(inputOutputFile(file, file2) == -1){
                exit(1);  // terminates child if an error occurs
            }
        } 
        if (strcmp(job, "error") == 0) {
            exit(1);  // terminates child when an error occurs while getting the job(I/O operation tyep (append, trunc, ...))
        } 
        if (execv(commandPath, args) == -1 && args[0] != NULL) { // tries to execute command
            fprintf(stderr, "\n%s: command was not found.\n\n", command);
        }
        exit(1); // child terminates itself if it escapes exec when an error occurs
    }
    if(strcmp(commandPath, "") != 0 && *background) { // if the command is found and is running in the background
        addProc(fullCommand, id, runningProc, processIndex);  // add the new created process to the array of running processes
    }
    if((strcmp(commandPath, "") != 0) && (!*background)) {
        currentlyRunningProc = id;
    }else {
        currentlyRunningProc = 0;
    }
    if(!*background) {  // checks if user wants to run the process in the background
        wait(NULL); // waits for process to finish
    }
    strcpy(commandPath, "");  // clears the commandPath string

}

int search(const char *text, char *rec, char *startPath, int *foundResult) {
    DIR *currDir = opendir(startPath);  // holds the start path, virtually everytime, "./" will be passed
    struct dirent *file;  // folds the files' and folders' information in currDir 
    FILE *fp;
    ssize_t content; // holds the content of the file
    size_t len = 0;  // holds the len of the file
    char  *line;    // holds each line in a file
    int lineNumber = 0;  // holds the number of line at each line in the file
    char *foundText;  // holds the searched text
    char path[100]; // holds the path of each file/folder
    while((file = readdir(currDir)) != NULL) {  // goes through the give directory
        if((strcmp(file->d_name, ".") == 0 ) || (strcmp(file->d_name, "..") == 0)) {
            continue;  // if the file/folder is the current or parent, skip that
        }
        lineNumber =0;  // resets the line number to 0 for each file
        strcpy(path, startPath);  // copy path to the current path
        strcat(path, file->d_name);  // concat the path with file name or folder name
        if((strstr(file->d_name, ".c") !=NULL) || (strstr(file->d_name, ".h") != NULL) || // if the files are c or h files
            (strstr(file->d_name, ".C") !=NULL) || (strstr(file->d_name, ".H") != NULL)) {
            fp = fopen(path, "r");  // open the file and read it
            if(fp == NULL){  // if the current path is not a file, continue
                continue;
            }
            while ((content = getline(&line, &len, fp)) != -1) { // reads the file
                lineNumber ++;
                if((foundText = strstr(line, text)) != NULL) { // if the searched text is in the file
                    char *text = line;
                    while (isspace(*text)) // remove tabs and spaces
                        ++text;
                    printf("\t%d: %s%s -> %s\n",lineNumber, startPath, file->d_name, text); 
                    *foundResult = 1;  // sets foundResult to 1 (true)
                }
            }
            fclose(fp);  // closes file
        }
        strcat(path, "/");  // concatenates path with /
        if(strcmp(rec,"-r") == 0) {  // if -r is given then run the function recursively
            DIR *subDir = opendir(path);
            if(subDir != NULL) {
                search(text, rec, path, foundResult);
            }
            closedir(subDir);  // closes the sub direcotries

        }

    }
    closedir(currDir); // closes the main directory

}

void removeAndSym(char *args[]) {  // used to remove the & symbol from the arguements
    int i = 0;
    while(args[i] != NULL) {
        if(strcmp(args[i], "&") == 0) {
            args[i] = NULL;
        }
        i++;
    }
}

int getLenArr(char *args[]) {  // gets the length of args array
    int counter = 0;
    while(args[counter] != NULL) {
        counter++;
    }
    return counter;
}

int checkIO(char *args[], char job[]) { // checks if the input is an I/O operatino
    int counter = 1;
    while(args[counter] != NULL) {
        if((strcmp(args[counter], ">>") == 0) || (strcmp(args[counter], "<") == 0) ||
        (strcmp(args[counter], "2>") == 0) || (strcmp(args[counter], ">") == 0)) {
            return 1;
        }
        counter++;
    }
    strcpy(job, "None"); // if it is not then set job to none
    return 0;
}

int getIOInfo(char *args[], char job[], char file[], char file2[]) {  // get I/O information from the user's input
    int counter = 1;  // used to loop through the args
    int indexOfInput = 0;  // used to distinguish input from input (< input) and output (< input > output)
    strcpy(file, "None");  // initializes file as empty
    strcpy(file2, "None");
    if(getLenArr(args) < 3) { // if there is only one arguement give error 
        fprintf(stderr, "%s\n", "\nAn error occured. Please specify a file");
        strcpy(job, "error");
        return 1;
    }
    while(args[counter] != NULL) {  // loop through arsg

        if (strcmp(args[counter], ">>") == 0) {
            strcpy(job, "append");  // change job value
            strcpy(file, args[counter+1]);
            break;
        }
        else if (strcmp(args[counter], "<") == 0) {
            strcpy(job, "input");
            indexOfInput = counter;  // set indexOfInput to the index of the give file
            strcpy(file, args[counter+1]);  // gets the input file name
            break;
        }
        else if (strcmp(args[counter], "2>") == 0) {
            strcpy(job, "error");
            strcpy(file, args[counter+1]);
        }
        else if(strcmp(args[counter], ">") == 0) {
            strcpy(job, "trunc");
            strcpy(file, args[counter+1]);
            break;
        }
        counter++;  
    }

    if(getLenArr(args) == 4 && indexOfInput != 0) { // if there are three args and there is an indexOfinput
        strcpy(job, "error");  // then set it as error
        fprintf(stderr, "\nAn error occured, Please make sure you provided the outpute file\n");
        return 1;
    }
    else if(getLenArr(args) > 4 && indexOfInput != 0) {
        if(strcmp(args[indexOfInput], "<") == 0 && 
            strcmp(args[indexOfInput+2], ">") == 0) {
                strcpy(job, "inputWrite");
                strcpy(file, args[indexOfInput+1]); // gets the input file name
                strcpy(file2, args[indexOfInput+3]);  // gets the output file name
        }
    }
}

void ignoreIOchar(char *args[]) {  // just like the ignore & function this ignores the IO characters like > < >> and 2>
    int i = 0;
    while(args[i] != NULL) {
        if(strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0 || 
            strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0  ) {
            args[i] = NULL;
        }
        i++;
    }
}

int searchInput(char *args[], char searchWord[], int *isRec) {
    int i = 1;
    *isRec = 0; // used to tell if the -r is given
    printf("%d\n", getLenArr(args));
    strcpy(searchWord, "");
    if(getLenArr(args) > 3) { // if there are more args than needed print and error
        fprintf(stderr, "\nAn error has occured. Please only use one search word\n\n");
        return -1;
    }
    if(getLenArr(args) < 2) {  // if there are less args that needed print error
        fprintf(stderr, "\nAn error has occured. Please specify a search word\n\n");
        return -1;
    }
    while(args[i] != NULL) { 
        if(strcmp(args[i], "-r") == 0) {  // if the -r is given then set isRec to 1 (true)
            *isRec = 1;
        }else{
            strcpy(searchWord, args[i]);  //  otherwise get the search word info
        }
        i++;
    }

    if(strcmp(searchWord, "") == 0) {  // prints an error if no word is specified
        fprintf(stderr, "\nAn error has occured. You must specify a search word with -r\n\n");
        return -1;
    }
}

void addBookmark(int *indx, char command[]) {
    bookmark newBookmark;  // creates a new bookmark 
    newBookmark.indx = *indx;
    newBookmark.exists = 1;  // used to tell if the bookmark exists in the array
    strcpy(newBookmark.name, command);
    bookmarked[*indx] = newBookmark; // adds the bookmark to the array
    ++*indx;


}

void viewBookmarks() {  // loops throught the array and prints the info
    int counter = 0;
    while (bookmarked[counter].exists != 0) {
        printf("\t%d %s\n", bookmarked[counter].indx, bookmarked[counter].name);
        counter++;
    }
}

int getLenBookmarked(){  // get the length of the bookmarked array
    int counter = 0;
    while(bookmarked[counter].exists == 1) {
        counter++;
    }
    return counter;
}


int bookmarkInput(char *args[], char command[], char bmArgs[]) {
    int counter = 1;
    int arrLen = getLenArr(args);
    strcpy(bmArgs, ""); // will hold bookmark args like -l
    strcpy(command, "");  // will hold the command
    if(arrLen == 1) {
        fprintf(stderr, "\nAn error has occured. Please specify a command to bookmark\n\n");
        return -1;
    }
    while(args[counter] != NULL) {
        if(strstr(args[counter], "\"") != NULL) {  // gets the command to be bookmarked
            strcat(command, args[counter]);
            strcat(command, " ");
        }
        else if(strcmp(args[counter], "-l") == 0) {  // gets the arg
            strcpy(bmArgs, args[counter]);
        } 
        else if(strcmp(args[counter], "-d") == 0) {  // get the arg
            strcpy(bmArgs, args[counter]);
            if(arrLen < 3) {  // prints error when no index is specified
                fprintf(stderr, "\nAn error has occured. You need to specify an index\n\n");
                return -1;
            }
            strcpy(command, args[counter +1]);
        }
        else if(strcmp(args[counter], "-i") == 0) {
             if(arrLen < 3) {  // prints error when no index is specified
                fprintf(stderr, "\nAn error has occured. You need to specify an index\n\n");
                return -1;
            }

            if(getLenBookmarked(bookmarked) < 1) {  // prints error when are no bookmarked commands to delete
                fprintf(stderr, "\nThere are currenlty no bookmarked commands\n\n");
                return -1;
            }           
            strcpy(bmArgs, "-i");
            strcpy(command, args[counter +1]);

        }
        counter++;
    }
    if(strcmp(command, "") == 0 && strcmp(bmArgs, "") == 0 ) {  // prints error when the command is not in quotes
        fprintf(stderr, "\nAn error has occured. Please make sure you've entered the command in quotes\n\n");
        return -1;
    }
}


void updateBookmarked(int startingIndx) {
    int counter = 0;
    while(bookmarked[counter].exists == 1) { // copies bookmark info to another array
        bookmark newCopy;
        newCopy.indx = bookmarked[counter].indx;
        strcpy(newCopy.name, bookmarked[counter].name);
        newCopy.exists = 1;
        CopiedBookmark[counter] = newCopy;
        counter++;
    }
    counter = startingIndx;
    while(CopiedBookmark[counter].exists == 1){  // shifts the array sideways (to left) when item is deleted
        if(CopiedBookmark[counter+1].exists == 1) {
            bookmarked[counter].indx = CopiedBookmark[counter+1].indx - 1;
            strcpy(bookmarked[counter].name, CopiedBookmark[counter+1].name);            
        }
        counter++;

    }
    if(bookmarked[counter-1].exists == 1){  // if a duplicate item exists remove it
        bookmarked[counter-1].exists = 0;
        CopiedBookmark[counter-1].exists = 0;
    }



}

int deleteBookmarks(char const *indx) {
    int counter = 0;
    int indxAsInt = atoi(indx);  // converts index to integer
    int resultIsFound = 0;  // tells if result was found
    while(bookmarked[counter].exists != 0) {
        if(indxAsInt == bookmarked[counter].indx) {
            resultIsFound = 1; 
        }
        counter++;
    }
    if(!resultIsFound) {  // if result not found print error
        fprintf(stderr, "\nAn error occured. There isn't a command at index %d\n\n",indxAsInt);
        return -1;
    }
    updateBookmarked(indxAsInt);
}

int getCommand(char indx[]){
    int indxInt = atoi(indx);  // convert index to integer
    if(indxInt > getLenBookmarked(bookmarked) -1) {
        return -1;
    }
    int i = 0;
    int commandFound = 0;
    while (bookmarked[i].exists == 1) {
        if(bookmarked[i].indx == indxInt){
            strcpy(indx, bookmarked[i].name); // gets the name of command given index of bookmark
            commandFound = 1;
        }
        i++;
    }
}

int execBookmark(char command[], char *args[], char *inputBuffer) {
    char *text = strtok(command, "\"");
    char *arg;
    int i = 0;
    arg = strtok(text, " "); // gets the args of bookmark
    while(arg != NULL){
        strcpy(args[i], arg);
        arg = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL; // sets unwanted arg to null

    strcpy(inputBuffer, args[0]);  // gets the name of the command
 }

void handle_sigtstp(int sig) {
    if(currentlyRunningProc != 0) {  // checks if there is a process that is running
        kill(currentlyRunningProc, SIGTSTP);
    }

}


int main(void)
{
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    char foundPath[MAX_PATH]; // holds the given commmand's path
    char *pathEnv= getenv("PATH"); // gets the path env variable
    int processIndex = 1; // used to assign a process an index
    char jobIO[INPUT_MAX]; // used to tell which I/O operation (job) to do
    char IOFile[INPUT_MAX]; // contains the info of first file in I/O operation
    char IOFile2[INPUT_MAX];  // contains the info of the second file in the I/O operation
    char searchWord[INPUT_MAX]; // contains the searched word in the search function
    char fullCommandName[INPUT_MAX];  // contains the command's name and its arguements
    int isRecursive = 0;  // used to tell whether the function should be ran recursively or not
    int foundResult = 0;  // used to tell whether the result was found
    int bookmarkIndex = 0;  // used to tell which bookmarked command to use with its index
    char bookmarkArgs[PROC];  // contains the bookmark arguements
    int giveBookmarkExec = 0; // when a bookmark is run, skip the setup function and conitues exec
    struct sigaction sa;  // signal handling
    sa.sa_handler = &handle_sigtstp;
    sigaction(SIGTSTP, &sa, NULL);  // calls the hand_sigstp function when ^Z is detected
    while (1){  
                background = 0;
                printf("myshell: ");
                /*setup() calls exit() when Control-D is entered */
                if(!giveBookmarkExec) {
                    strcpy(inputBuffer, "");
                    setup(inputBuffer, args, &background);
                    if(args[0] == NULL) {
                        continue;
                    }
                }
                else {
                    giveBookmarkExec = 0;
                }
                if(strcmp(inputBuffer, "ps_all") == 0) {
                    checkOnChildren();  // checks on the children when ps_all is ran
                    ps_all();
                }
                else if(strcmp(inputBuffer, "search") == 0) {
                    if (searchInput(args, searchWord, &isRecursive) != -1) {
                        char *text = strtok(searchWord, "\"");  // removes the quotaion marks
                        if(isRecursive) {
                            search(text, "-r", "./", &foundResult);  // runs the search recursively
                            if(!foundResult){
                                fprintf(stderr, "\n%s was not found in any of the files\n", text);  // handle errors
                            }

                        }else{
                            search(text, "n", "./", &foundResult);  // runs the search non-recursively
                            if(!foundResult){
                                fprintf(stderr,"\n%s was not found in any of the files\n", text);  // handle errors
                            }
                        }
                    }                    
                }
                else if(strcmp(inputBuffer, "exit") == 0) {
                    checkOnChildren();  // checks on the background process before exiting
                    if(checkArrsEmpty(0) == 'F') { // if there are running processes print error
                        printf("\nThere are still processes running in the background\nPlease terminate them before you exit\n\n");
                    }else {  // else quit
                        exit(0);
                    }
                }
                else if(strcmp(inputBuffer, "bookmark") == 0) {
                    char bookmarkCommand[INPUT_MAX]; // contains the bookmark command
                    if(bookmarkInput(args, bookmarkCommand, bookmarkArgs) != -1) {  // gets the input
                        if(strcmp(bookmarkArgs, "-l") == 0) {  // if -l is given view the bookmarks
                            viewBookmarks();
                        }
                        else if (strcmp(bookmarkArgs, "-d") == 0) {  // if bookmark is deleted, remove it from the bookmark
                            if(deleteBookmarks(bookmarkCommand) != -1) {
                                --bookmarkIndex;  // and decrease the bookmark index
                            }
                        }
                        else if (strcmp(bookmarkArgs, "-i") == 0) {  // if a bookmark is ran it will exec it
                            if(getCommand(bookmarkCommand) != -1){
                                execBookmark(bookmarkCommand, args, inputBuffer);
                                giveBookmarkExec = 1;
                            }else{
                                fprintf(stderr, "\nAn error occured. There isn't a command at this index\n\n");
                            }
                        }
                        else {
                            addBookmark(&bookmarkIndex, bookmarkCommand);  // else if no args were given add the given command

                        }
                    }
                    
                }
                else {
                    removeAndSym(args); // ignore & symbol
                    int i = 1;
                    strcpy(fullCommandName, "");
                    strcat(fullCommandName, inputBuffer);
                    while(args[i] != NULL){ // gets the commnad and its arguement then stors them in a variable
                        strcat(fullCommandName, " ");
                        strcat(fullCommandName, args[i]);
                        i++;
                    }
                    if(checkIO(args, jobIO)) {  // if I/O operation is requested
                        getIOInfo(args, jobIO, IOFile, IOFile2); // get the info of input
                    }
                    ignoreIOchar(args); // ignore I/O symbols
                    findCommandEnv(foundPath, inputBuffer, pathEnv); // finds the given command's path from the PATH variable
                    createProccess(foundPath, args, &background, inputBuffer, &processIndex, jobIO,
                        IOFile, IOFile2, fullCommandName);
                    currentlyRunningProc = 0;


                }
    }
}

