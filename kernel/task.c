#include "task.h"
#include "mmu.h"
#include "kmalloc.h"
#include "serial.h"
#include "list.h"
#include "mmu.h"

#define NO_TASKS 64 //Max 64 tasks for now
#define STACK_SIZE 4096

extern void set_kernel_stack(size_t);

task_t task_list[NO_TASKS];
task_t *current = NULL;
list_t *ready_queue = NULL;
task_t *kernel_idle_task = NULL;

uint32_t last_process = 0;

#define PUSH(stack, type, value)                                               \
	stack -= sizeof(type);                                                 \
	*((type *)stack) = value

void exit_task()
{
	kpanic_fmt("TASK RETURN");
	while (1)
		;
}
void idle_task() {
	while(1) {
		schedule();
	}
}
extern page_directory_t *current_directory;
task_t *spawn_init() {
	task_t *init_task = kcalloc(sizeof(task_t));

	init_task->state = STATE_RUNNING;

	init_task->stack = (vptr_t)kmalloc(STACK_SIZE); //TODO, reuse kernel stack?
	init_task->page_directory = current_directory;

	return init_task;
}

task_t *spawn_idle() {
	return copy_task((vptr_t)idle_task, NULL);
}
void tasking_install() {
	ready_queue = list_create();
	current = spawn_init();
	kernel_idle_task = spawn_idle();
}

//Creates a new "thread" that runs the function fn
//Also allocates a new stack
//Does not start it yet
task_t *copy_task(vptr_t fn, vptr_t args)
{
	task_t *new_task = &task_list[++last_process];

	//Clear the old data
	memset((void *)new_task, 0, sizeof(task_t));

	vptr_t stack = (vptr_t)(kmalloc(STACK_SIZE) + STACK_SIZE);
	new_task->stack = stack;

	PUSH(stack, vptr_t, args);
	//Where fn returns to
	PUSH(stack, vptr_t, (vptr_t)exit_task);

	new_task->context.eip = fn;
	new_task->context.esp = stack;

	new_task->page_directory = clone_directory(current->page_directory);

	return new_task;
}
extern void enter_userspace(vptr_t fn, vptr_t stack);
void exec(task_t *task, vptr_t fn)
{
	vptr_t stack = (vptr_t)kmalloc(0x1000);
	enter_userspace(stack, fn);
}
void make_task_ready(task_t *task)
{
	if(task == kernel_idle_task) {
		return;
	}
	
	task->state = STATE_READY;
	list_append_item(ready_queue, (vptr_t)task);
}

task_t *pick_next_task()
{
	if(ready_queue->len == 0) {
		return kernel_idle_task;
	}

	node_t *task_node = list_index(ready_queue, 0);
	task_t *task = (task_t*) task_node->value;

	list_remove(ready_queue, task_node);

	return task;
}
void schedule()
{
	task_t *next_task = pick_next_task();
	make_task_ready(current);
	if (next_task == current) {
		return;
	}

	context_t *old_context = (context_t *)&current->context;

	//Set up everything for the new process
	current = next_task;
	set_kernel_stack(current->stack);

	switch_page_directory(current->page_directory);

	switch_context(old_context, &current->context);
}