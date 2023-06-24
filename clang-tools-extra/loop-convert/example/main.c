#include "a.h"

void mutex_lock(struct mutex* mutex)
{

}

void do_something(struct B* b_ptr)
{
}



int main() {
    struct A a;
    mutex_lock((&a.b_ptr->mutex));
    do_something(a.b_ptr);

    return 0;
}