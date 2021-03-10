#include "analysis.h"
#include "model.h"
#include "action.h"
#include "execution.h"

void Analysis::ERROR(ModelAction * action, const char * message) {
    if(action && action->get_position()) {
        if(errorSet.get(action) == NULL) {
            model->get_execution()->add_bug("%s: %s ====> address=%p \t location=%s\n",getName(), message, 
                            action->get_location(), action->get_position());
            errorSet.add(action);
        }        
    } else {
        model->get_execution()->add_bug("%s: %s ====> address %p\n",getName(), message, action->get_location());
    }

}

unsigned int hashErrorPosition(ModelAction *a) {
    const char * position = a->get_position();
    unsigned int hash = 0;
    while(position != NULL) {
        hash = (hash << 2) ^ (unsigned int)(*position);
        position++;
    }
    return hash;
}

bool equalErrorPosition(ModelAction *a1, ModelAction *a2) {
    if(a1 == NULL || a2 == NULL || a1->get_position() == NULL || a2->get_position() == NULL) {
        return false;
    }
    return strcmp(a1->get_position(), a2->get_position()) == 0;
}