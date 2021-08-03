#include "analysis.h"
#include "model.h"
#include "action.h"
#include "execution.h"

const char * duplicateString(const char * str) {
	char *copy = (char*)model_malloc(strlen(str) + 1);
	strcpy(copy, str);
	return (const char *)copy;
}

void Analysis::ERROR(ModelExecution *exec, ModelAction * wrt, ModelAction *read, const char * message) {
	if(wrt->get_position()) {
		if(errorSet.get(wrt->get_position()) == NULL) {
			ASSERT(read && read->get_position());
			model->get_execution()->add_warning("ERROR: %s: %s ====> write: Seq_number=%u \t Execution=%p \t Address=%p \t Location=%s\t"
																					">>>>>>> Read by: Address=%p \t Location=%s\n",getName(), message, wrt->get_seq_number(),
																					exec, wrt->get_location(), wrt->get_position(), read->get_location(), read->get_position());
			errorSet.add(duplicateString(wrt->get_position()));
		}
	} else {
		if(read->get_position()) {
			if(errorSet.get(read->get_position()) == NULL) {
				model->get_execution()->add_warning("ERROR: %s: %s ====> write: Seq_number=%u \t Execution=%p \t Address=%p\t"
																						">>>>>>> Read by: Address=%p \t Location=%s\n",getName(), message, wrt->get_seq_number(),
																						exec, wrt->get_location(), read->get_location(), read->get_position());
				errorSet.add(duplicateString(read->get_position()));
			}
		} else {
			model->get_execution()->add_warning("ERROR: %s: %s ====> Seq_number=%u \t Address %p\n",getName(), message, wrt->get_seq_number(), wrt->get_location());
		}
	}
	num_total_bugs++;
}

void Analysis::WARNING(ModelExecution *exec, ModelAction * wrt, ModelAction *read, const char * message) {
	if(wrt->get_position()) {
		if(warningSet.get(wrt->get_position()) == NULL) {
			ASSERT(read && read->get_position());
			model->get_execution()->add_warning("WARNING: %s: %s ====> write: Seq_number=%u \t Execution=%p \t Address=%p \t Location=%s\t"
																					">>>>>>> Read by: Address=%p \t Location=%s\n",getName(), message, wrt->get_seq_number(),
																					exec, wrt->get_location(), wrt->get_position(), read->get_location(), read->get_position());
			warningSet.add(duplicateString(wrt->get_position()));
		}
	} else {
		if(read->get_position()) {
			if(warningSet.get(read->get_position()) == NULL) {
				model->get_execution()->add_warning("WARNING: %s: %s ====> write: Seq_number=%u \t Execution=%p \t Address=%p\t"
																						">>>>>>> Read by: Address=%p \t Location=%s\n",getName(), message, wrt->get_seq_number(),
																						exec, wrt->get_location(), read->get_location(), read->get_position());
				warningSet.add(duplicateString(read->get_position()));
			}
		} else {
			model->get_execution()->add_warning("WARNING %s: %s ====> Seq_number=%u \t Address %p\n",getName(), message, wrt->get_seq_number(), wrt->get_location());
		}
	}
	num_total_warnings++;
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
