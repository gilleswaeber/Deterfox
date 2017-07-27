#ifndef vm_counter_h
#define vm_counter_h
#include "stdio.h"

//#define DEBUG_JS_COUNTER

#ifdef DEBUG_JS_COUNTER
#define JS_COUNTER_LOG(...) do {\
	//printf("[DEBUG, %s, %i, %s]", __FILE__, __LINE__, __FUNCTION__);\
	//printf(__VA_ARGS__);\
	//printf("\n");\
}while(0)
#else
#define JS_COUNTER_LOG(...)
#endif

extern volatile uint64_t counter;
extern volatile uint64_t physical_base;
extern bool isSystem;
extern bool cross_origin;

void inc_counter(uint64_t args, void* key = NULL);
uint64_t get_counter(void* key = NULL);
bool set_counter(uint64_t time, void* key = NULL);
uint64_t get_scaled_counter(uint64_t args);

uint64_t getPhysicalBase();

uint64_t getPhysicalTime();

void set_synchronize(bool flag, void* key = NULL);

bool get_synchronize(void* key = NULL); 

void set_inckey(void* key);

#endif
