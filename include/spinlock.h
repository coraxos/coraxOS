#pragma once

void spinlock_acquire(int *lock);
void spinlock_release(int *lock);