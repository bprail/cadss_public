#include "config.h"

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

char* configContents = NULL;
size_t length = 0;

struct element {
    char* name;
    int argCount;
    char** argList;
    struct element* next;
};

struct element* componentList = NULL;

const size_t ARG_LIST_CHUNK_SIZE = 16;

void printSettings()
{
    struct element* current = componentList;
    
    fprintf(stderr, "Printing Component configurations:\n");
    while (current != NULL)
    {
        fprintf(stderr, "  __ %s  (arg count - %d)\n", current->name, current->argCount);
        size_t c = 0;
        while (current->argList[c] != NULL)
        {
            fprintf(stderr, "%ld - %s\n", c, current->argList[c]);
            c++;
        }
        
        current = current->next;
    }
}

//
//  parseSettings
//
//   A destructive parsing of the settings string to identify each argument,
// insert '\0', and also provide arrays of arguments for the components.
//
static void parseSettings()
{
    struct element* current = NULL;
    int argSize = ARG_LIST_CHUNK_SIZE;
    
    size_t pos = 0;
    int inQuote = 0;
    int inComment = 0;
    int inMultiComment = 0;
    do {
        if (inComment == 1)
        {
            while (pos < length && configContents[pos] != '\n')
            {
                pos++;
            }
            inComment = 0;
            continue;
        }
        
        if (inMultiComment == 1)
        {
            while (pos < length &&
                   (configContents[pos] != '*' && 
                    configContents[pos + 1] != '/'))
            {
                pos++;
            }
            if (pos == length)
            {
                fprintf(stderr, "Warning - multiline comment trailed to end of file\n");
            }
            else
            {
                pos += 2;
            }
            inMultiComment = 0;
            continue;
        }
        
        // As we know the last character is '\0', a non-NULL
        //   character will always have another character after
        //   it.  And that configContents[length] == '\0'.
        if (configContents[pos] == '/')
        {
            if (configContents[pos + 1] == '/')
            {
                inComment = 1;
                pos += 2;
            }
            else if (configContents[pos + 1] == '*')
            {
                inMultiComment = 1;
                pos += 2;
            }
            continue;
        }
        
        while ( pos < length && isspace(configContents[pos]))
        {
            pos++;
        }
        if (pos == length) break;
        
        if (configContents[pos] == '_' && configContents[pos + 1] == '_')
        {
            pos += 2;
            size_t compNameStart = pos;
            while ( pos < length && !isspace(configContents[pos]))
            {
                pos++;
            }
            if (pos == length)
            {
                fprintf(stderr, "Component name trailed to end of file\n");
                break;
            }
            configContents[pos] = '\0';
            pos++;
            
            char* componentName = strdup(&configContents[compNameStart]);
            struct element* nextComp = malloc(sizeof(struct element));
            if (componentName == NULL || nextComp == NULL)
            {
                
                break;
            }
            
            nextComp->name = componentName;
            nextComp->next = NULL;
            nextComp->argCount = 1;
            nextComp->argList = malloc(sizeof(char*) * ARG_LIST_CHUNK_SIZE);
            nextComp->argList[0] = componentName;
            nextComp->argList[1] = NULL;
            argSize = ARG_LIST_CHUNK_SIZE;
            if (current == NULL)
            {
                current = nextComp;
                componentList = nextComp;
            }
            else
            {
                current->next = nextComp;
                current = nextComp;
            }
            
            continue;
        }
        
        if (current == NULL)
        {
            fprintf(stderr, "Failed to start with either comments or component");
            return ;
        }
        
        //
        // What follows is an argument
        //
        if (configContents[pos] == '"')
        {
            inQuote = 1;
            pos++;
        }
        size_t argPosStart = pos;
        do {
            if (inQuote == 1)
            {
                while (pos < length && configContents[pos] != '"')
                {
                    pos++;
                }
                inQuote = 0;
                break;
            }
            if (isspace(configContents[pos])) break;
            
            if (configContents[pos] == '/')
            {
                if (configContents[pos + 1] == '/')
                {
                    inComment = 1;
                    break;
                }
                if (configContents[pos + 1] == '*')
                {
                    inMultiComment = 1;
                    break;
                }
            }
            
            pos++;
        } while (pos < length);
        
        if (argPosStart != pos)
        {
            //fprintf(stderr, "Copying from %zd to %zd, %c - %c\n", argPosStart, pos,
            //                configContents[argPosStart], configContents[pos]);
            
            configContents[pos] = '\0';
            char* arg = strdup(&configContents[argPosStart]);
            
            if (current->argCount == argSize)
            {
                argSize += ARG_LIST_CHUNK_SIZE;
                current->argList = realloc(current->argList, sizeof(char*) * argSize);
            }
            current->argList[current->argCount] = arg;
            current->argCount++;
            current->argList[current->argCount] = NULL;
        }
        
        if (inComment == 1 || inMultiComment == 1)
        {
            pos++;
        }
        
        pos++;
    } while (pos < length);
    
}

int openSettings(char* configName)
{
    int configDesc = open(configName, O_RDONLY);
    if (configDesc == -1)
    {
        perror("Opening Settings file");
        return -1;
    }
    
    struct stat sb;
    if (fstat(configDesc, &sb) == -1)
    {
        perror("Getting settings file size");
        close(configDesc);
        return -1;
    }
    length = sb.st_size;
    
    char* configContentsTemp = mmap(NULL, length, PROT_READ, MAP_PRIVATE, configDesc, 0);
    if (configContentsTemp == NULL)
    {
        perror("Reading settings file contents");
        close(configDesc);
        return -1;
    }
    
    // N.B. As getopt needs to permute the arguments, the configuration
    //   is copied locally so that any change is not reflected in the
    //   on disk file.
    configContents = malloc(length + 1);
    if (configContents == NULL)
    {
        perror("Allocating local space for settings arguments");
        munmap(configContentsTemp, length);
        close(configDesc);
        return -1;
    }
    memcpy(configContents, configContentsTemp, length);
    configContents[length] = '\0';
    
    munmap(configContentsTemp, length);
    close(configDesc);
    
    parseSettings();
    
    //printSettings();
    
    return 0;
}

char** getSettings(char* componentName, int* count)
{
    *count = 0;
    
    if (componentName == NULL) 
    {
        fprintf(stderr, "No component name specified\n");
        return NULL;
    }
    
    if (componentList == NULL)
    {
        fprintf(stderr, "Configuration file not opened or parsed\n");
        return NULL;
    }
    
    struct element* e = componentList;
    while (e != NULL)
    {
        if (strcmp(e->name, componentName) == 0) break;
        
        e = e->next;
    }
    
    if (e == NULL)
    {
        fprintf(stderr, "Component name - %s was not found in settings\n", componentName);
        return NULL;
    }
    
    *count = e->argCount;
    return e->argList;
}

void freeSettings()
{
    free(configContents);
}
