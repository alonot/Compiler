#include "../include/includes.h"



// Initialize the queue
Queue* init_queue() {
    Queue* q = (Queue*) (malloc(sizeof(Queue)));
    q->front = q->rear = NULL;
    q->len = 0;
}

// Enqueue an element
void enqueue_queue(Queue* q, lli value) {
    QNode* newQNode = (QNode*)malloc(sizeof(QNode));
    if (!newQNode) {
        printf("Memory allocation failed\n");
        return;
    }
    newQNode->data = value;
    newQNode->next = NULL;
    q->len ++;

    if (q->rear == NULL) { // Empty queue
        q->front = q->rear = newQNode;
    } else {
        q->rear->next = newQNode;
        q->rear = newQNode;
    }
}

// Dequeue the front element
lli dequeue_queue(Queue* q) {
    if (q->front == NULL) {
        return LONG_MIN;
    }

    QNode* temp = q->front;
    q->len --;
    lli val = q->front->data;
    q->front = q->front->next;
    if (q->front == NULL) // Queue became empty
        q->rear = NULL;
    free(temp);
    return val;
}

// Remove first instance of a value
lli remove_item(Queue* q, lli value) {
    QNode *curr = q->front, *prev = NULL;

    while (curr != NULL) {
        if (curr->data == value) {
            q->len --;
            if (prev == NULL) { // First Qnode
                q->front = curr->next;
                if (q->rear == curr) // Only one Qnode
                    q->rear = NULL;
            } else {
                prev->next = curr->next;
                if (q->rear == curr)
                    q->rear = prev;
            }
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return 1;
}

void free_queue(Queue* q) {
    QNode* current = q->front;
    while (current != NULL) {
        QNode* temp = current;
        current = current->next;
        free(temp);
    }
    q->front = q->rear = NULL;
    free(q);
}