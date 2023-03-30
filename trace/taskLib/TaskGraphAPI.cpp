#include "TaskGraph.hpp"

#include "TaskGraphAPI.h"

#include <string.h>

contech::TaskGraph* tg = NULL;

int contextCount = 1;

struct taskTrack {
  bool isComplete;
  contech::TaskId tid;
  contech::Task* t;  
  contech::Task::memOpCollection moc;
  contech::Task::memOpCollection::iterator mit, met;
};

taskTrack* currentTasks;

void debugPrint()
{
    std::cout << "Contexts: " << contextCount << std::endl;
    for (int i = 0; i < contextCount; i++)
    {
        std::cout << "Proc: " << i << " \t Complete: " << currentTasks[i].isComplete << endl;
        std::cout << "Last TID: " << currentTasks[i].tid << endl;
        if (currentTasks[i].t != NULL)
        {
            std::cout << *currentTasks[i].t;
        }
        else
        {
            std::cout << "NULL Task" << endl;
        }
    }
    
}

int8_t initTaskGraph(FILE* tf)
{
    tg = contech::TaskGraph::initFromFile(tf);
    if (tg == NULL) return -1;
    
    contextCount = tg->getNumberOfContexts();
    
    currentTasks = (taskTrack*) calloc(contextCount, sizeof(taskTrack));
    
    return 1;
}

void updateContext(int processorNum)
{
    if (currentTasks[processorNum].isComplete == true) return;
    if (currentTasks[processorNum].t != NULL) delete currentTasks[processorNum].t;
    
    currentTasks[processorNum].t = tg->getTaskById(currentTasks[processorNum].tid);
        
    contech::Task* t = currentTasks[processorNum].t;
    
    if (t == NULL)
    {
        currentTasks[processorNum].isComplete = true;
        return;
    }
    
    if (t->getType() != contech::task_type_basic_blocks)
    {
        currentTasks[processorNum].tid = currentTasks[processorNum].tid.getNext();
        updateContext(processorNum);
        return;
    }
    
    auto v = t->getActions();
    if (v.empty() == true)
    {
        currentTasks[processorNum].tid = currentTasks[processorNum].tid.getNext();
        updateContext(processorNum);
        return;
    }
    
    currentTasks[processorNum].moc = t->getMemOps();
    currentTasks[processorNum].mit = currentTasks[processorNum].moc.begin();
    currentTasks[processorNum].met = currentTasks[processorNum].moc.end();
}

trace_op* getNextOp(int processorNum)
{
    assert(processorNum >= 0 && processorNum < contextCount);
    
    if (currentTasks[processorNum].isComplete == true) return NULL;
    
    if (currentTasks[processorNum].t == NULL)
    {
        currentTasks[processorNum].tid = contech::TaskId(processorNum, 0);
        
        updateContext(processorNum);
    }
    
    {
        contech::Task* t = currentTasks[processorNum].t;
     
        if (t == NULL) return NULL;
     
        if (currentTasks[processorNum].mit == currentTasks[processorNum].met)
        {
            currentTasks[processorNum].tid = currentTasks[processorNum].tid.getNext();
            updateContext(processorNum);
            
            if (currentTasks[processorNum].t == NULL) return NULL;
        }
        
        contech::MemoryAction ma = *currentTasks[processorNum].mit;
        currentTasks[processorNum].mit++;
        
        trace_op* op = (trace_op*) calloc(1, sizeof(trace_op));
        if (op == NULL) return NULL;
        op->op = (ma.type == contech::action_type_mem_read)?MEM_LOAD:MEM_STORE;
        op->memAddress = ma.addr;
        op->size = (0x1 << ma.pow_size);
        op->src_reg[0] = -1;
        op->src_reg[1] = -1;
        op->dest_reg = -1;
        
        return op;
    }
}