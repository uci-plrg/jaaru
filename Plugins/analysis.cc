#include "analysis.h"
#include "model.h"
#include "action.h"
#include "execution.h"

const char * duplicateString(const char * str) {
    char *copy = (char*)model_malloc(strlen(str) + 1);
    strcpy(copy, str);
    return (const char *)copy;
}

void Analysis::ERROR(ModelExecution *exec, ModelAction * action, const char * message) {
    if(action && action->get_position()) {
        if(errorSet.get(action->get_position()) == NULL) {
            model->get_execution()->add_bug("%s: %s ====> Execution=%p \t Address=%p \t Location=%s\n",getName(), message, 
                            exec, action->get_location(), action->get_position());
            errorSet.add(duplicateString(action->get_position()));
        }        
    } else {
        model->get_execution()->add_bug("%s: %s ====> address %p\n",getName(), message, action->get_location());
    }

}

unsigned int hashErrorPosition(const char *position) {
    unsigned int hash = 0;
    uint32_t index = 0;
    while(position != NULL && position[index] != '\0') {
        hash = (hash << 2) ^ (unsigned int)(position[index++]);
    }
    return hash;
}

bool equalErrorPosition(const char *p1, const char *p2) {
    if(p1 == NULL || p2 == NULL) {
        return false;
    }
    return strcmp(p1, p2) == 0;
}