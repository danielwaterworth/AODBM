/*  
    Copyright (C) 2011 aodbm authors,
    
    This file is part of aodbm.
    
    aodbm is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aodbm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdlib.h"

#include "aodbm_stack.h"

struct aodbm_stack {
    aodbm_stack *up;
    void *dat;
};

void aodbm_stack_push(aodbm_stack **stack, void *data) {
    aodbm_stack *new = malloc(sizeof(aodbm_stack));
    new->dat = data;
    new->up = *stack;
    *stack = new;
}

void *aodbm_stack_pop(aodbm_stack **stack) {
    if (*stack == NULL) {
        return NULL;
    }
    aodbm_stack *next = (*stack)->up;
    void *out = (*stack)->dat;
    free(*stack);
    *stack = next;
    return out;
}
