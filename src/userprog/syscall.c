#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "devices/shutdown.h"

struct lock fs_lock;

struct t_file { 
  struct file *file; 
  int fd; /* File's file descriptor for the thread. */
  struct list_elem elem; /* File's entry in the open file list. */
}

static void syscall_handler (struct intr_frame *)
{
  typedef int syscall_function (int, int, int);
  /* A system call. */
  struct syscall
  {
    size_t arg_cnt; /* Number of arguments. */
    syscall_function *func; /* Implementation. */
  };

  /* Table of system calls. */
  static const struct syscall syscall_table[] =
  {
    {0, (syscall_function *) sys_halt},
    {1, (syscall_function *) sys_exit},
    {1, (syscall_function *) sys_exec},
    {1, (syscall_function *) sys_wait},
    {2, (syscall_function *) sys_create},
    {1, (syscall_function *) sys_remove},
    {1, (syscall_function *) sys_open},
    {1, (syscall_function *) sys_filesize},
    {3, (syscall_function *) sys_read},
    {3, (syscall_function *) sys_write},
    {2, (syscall_function *) sys_seek},
    {1, (syscall_function *) sys_tell},
    {1, (syscall_function *) sys_close}
  };
 
  const struct syscall *sc;
  unsigned call_nr;
  int args[3];

  /*Get the system call.*/
  copy_in (&call_nr, f->esp, sizeof call_nr);
  if(call_nr >= sizeof syscall_table / sizeof *syscall_table)
    thread_exit();
  sc = syscall_table + call_nr;

  /*Get the system call arguments.*/
  ASSERT (sc->arg_cnt <= sizeof args / sizeof *args);
  memset (args, 0, sizeof args);
  copy_in (args, (uint32_t *) f->esp + 1, sizeof *args * sc->arg_cnt);

  /*Execute the system call, and set the return value. */
  f->eax = sc->func(args[0], args[1], args[2]);
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&fs_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

void 
sys_halt (void)
{
  shutdown_power_off();
}

/* Incomplete. */
void 
sys_exit(int status)
{
  struct thread *current = thread_current();
  printf("%s: exit(%d)\n", cur->name, /*Figure out wait*/);
  thread_exit();
}

/* Incomplete. */
pid_t 
sys_exec(const char *cmd_line)
{

}

int 
sys_wait(pid_t pid)
{
  return process_wait(pid);
}

bool 
sys_create(const char *file, unsigned initial_size)
{
  lock_acquire(&fs_lock);
  bool val = filesys_create(file, initial_size);
  lock_release(&fs_lock);
  return val;
}

bool 
sys_remove(const char *file)
{
  lock_acquire(&fs_lock);
  bool success = filesys_remove(file);
  lock_release(&fs_lock);
  return success;
}

int 
sys_open(const char *file)
{
  lock_acquire(&fs_lock);
  struct file *f = filesys_open(file);
  /* Add file to file list. */
  struct t_file *tf = malloc(sizeof(struct t_file));
  tf->file = f;
  tf->fd = thread_current()->next_handle;
  thread_current()->next_handle++;
  list_push_back(&thread_current()->fds, &tf->elem);
  lock_release(&fs_lock)
  return tf->fd;
}

int 
sys_filesize(int fd)
{
  lock_acquire(&fs_lock);
  struct file *file = find_file(fd);
  int size = file_length(file);
  lock_release(&fs_lock);
  return size;
}

int 
sys_read(int fd, void *buffer, unsigned size)
{
  lock_acquire(&fs_lock);
  struct file *file = find_file(fd);
  int bytes = file_read(file, buffer, size);
  lock_release(&fs_lock);
  return bytes;
}

int 
sys_write (int fd, const void *buffer, unsigned size)
{
  lock_acquire(&fs_lock);
  struct file *file = find_file(fd);
  int bytes = file_write(file, buffer, size);
  lock_release(&fs_lock);
  return bytes;
}

void 
sys_seek(int fd, unsigned position)
{
  lock_acquire(&fs_lock);
  struct file *file = find_file(fd);
  file_seek(file, position);
  lock_release(&fs_lock);
}

unsigned 
sys_tell(int fd)
{
  lock_acquire(&fs_lock);
  struct file *file = find_file(fd);
  off_t os = file_tell(file);
  lock_release(&fs_lock);
  return os;
}

void
sys_close(int fd)
{
  lock_acquire(&fs_lock);
  struct thread *t = thread_current();
  for (e = list_begin (&t->fds); e != list_end (&t->fds); e = list_next (e))
  {
    struct t_file *tf = list_entry (e, struct t_file, elem);
    if(fd == tf->fd)
    {
      file_close(tf->file);
      list_remove(&tf->elem);
      free(tf);
    }
  }
  lock_release(&fs_lock);
}

struct file* find_file (int fd)
{
  struct thread *t = thread_current();
  struct list_elem *e;

  for (e = list_begin (&t->fds); e != list_end (&t->fds); e = list_next (e))
  {
    struct t_file *tf = list_entry (e, struct t_file, elem);
    if (fd == tf->fd)
      return tf->file;
  }
  return NULL;
}
