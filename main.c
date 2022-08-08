/*
 * Piotr Wojtowicz | Jagiellonian University | Cracow
 * This is an implementation of a popular
 * LRU algorithm which is a page-replacement
 * algorithm used in the virtual memory management system.
 *
 * We are going to fetch the data from a file that contains
 * the contiguous virtual address. We will extract
 * just parts that refer to the particular pages (we don't need
 * the offsets). We will also assume a 64-byte page size.
 */
#define ADDRESES "../adr.txt"
#define LOGS "./logs.txt"
/*
 * Hence:
 *  Virtual address     Virtual page number
 *  0x048                0x1
 *  0x080                0x2
 *  0x04E                0x1
 *  0x0FC                0x3
 *  0x1FC                0x7
 *  ...                  ...
 *
 * # File contains 4 digit addresses (i.e. 0x048 = 0048)#
 */
#define ADDR_LENGTH 4
/*
 * We will assume our system is capable of dealing
 * with just 5 addresses at the time
 * (in short, we have only 5 frames in our VM).
 * We will also assume our program will need an only
 * couple of addresses (no more than 15-20 - it's just
 * an exercise). These assumptions allow us to examine the
 * amount of page-fault exceptions.
 *
 */

/*
 *  Include the open system call
 */
#include <fcntl.h>

/*
 *  File stats
 */
#include <sys/stat.h>

/*
 * Read
 */
#include <unistd.h>

/*
 * bzero
 * strncpy
 */
#include <string.h>

/*
 * Perror
 */
#include <stdio.h>

/*
 * Exit
 */
#include <stdlib.h>

/*
 * Virtual address struct
 */
typedef struct{
    unsigned int virtual_page_number;
    unsigned int virtual_page_offset;
} VirtualMem;

#define VM_STRUCT_SIZE sizeof(VirtualMem)

/*
 * Allocate the memory
 */
void* Alloc_mem(size_t length, size_t type_size){
    void *ptr = NULL;
    if((ptr = calloc(length, type_size))==NULL){
        perror("Could not allocate memory");
        exit(EXIT_FAILURE);
    }
    /*
     * Clean the memory (just to be sure)
     */
    bzero(ptr, length);
    return ptr;
}

/*
 * Doubly inked list struct
 */
typedef struct Node{
    VirtualMem virtual_address;

    struct Node *prev;
    struct Node *next;
}Node;

Node *head = NULL;
Node *tail = NULL;

/*
 * I will implement only the most
 * necessary functions
 * Insert new element
 */

void Insert_top(VirtualMem vm_address){
    //If no elements
    if(head==NULL){
        head = tail = Alloc_mem(1, sizeof(Node));
        head->virtual_address.virtual_page_number = vm_address.virtual_page_number;
        head->virtual_address.virtual_page_offset = vm_address.virtual_page_offset;
        head->next = tail; tail->next = head;
        head->prev = tail; tail->prev = head;
        return;
    }
    //Allocate memory
    Node *new_node = Alloc_mem(1, sizeof(Node));
    new_node->virtual_address.virtual_page_offset = vm_address.virtual_page_offset;
    new_node->virtual_address.virtual_page_number = vm_address.virtual_page_number;
    //Set the next node to head
    new_node->next = head;
    //Set the prev node to tail
    new_node->prev = tail;
    //Set head prev to new 'head'
    head->prev = new_node;
    //Change the 'heads'
    head = new_node;
    tail->next = head;
}

/*
 * Linear time complexity
 * I don't use the hashmap
 */
Node* List_search(VirtualMem vm_data){

    Node *temp_ptr = head;
    if(head == NULL){
        Insert_top(vm_data);
        return head;
    }

    while(temp_ptr!=tail){
        if(temp_ptr->virtual_address.virtual_page_number == vm_data.virtual_page_number)
            return temp_ptr;

        temp_ptr = temp_ptr->next;
    }
    //check the tail
    return (temp_ptr->virtual_address.virtual_page_number == vm_data.virtual_page_number) ? temp_ptr : NULL;
}

/*
 * Delete nodes
 */
void Free_list(Node* list_head){
    //If list is empty
    if(head == NULL)
        return;
    Node *tmp_ptr = list_head->next;
    do{
        free(list_head);
        list_head = tmp_ptr;
        tmp_ptr = tmp_ptr->next;
    } while (list_head != head);
}

void Display_frames(Node* head_ptr){
    char tmp_text[29];
    int counter = 1;
    do{
        sprintf(tmp_text, "\nFRAME %2d | VPN: %2d | VPO: %2d",
                counter,
                head_ptr->virtual_address.virtual_page_number,
                head_ptr->virtual_address.virtual_page_offset);

        if(write(STDOUT_FILENO, tmp_text, sizeof(tmp_text)) == -1) {
            perror("Write error");
            exit(EXIT_FAILURE);
        }
        counter++;
        head_ptr = head_ptr->next;
    }while(head_ptr != head);

    if(write(STDOUT_FILENO, "\n=================\n", (sizeof("\n=================\n")-1))==-1){
        perror("Write error");
        exit(EXIT_FAILURE);
    }
}

void Renew_item(VirtualMem vm_data, int *is_full){
    /*
     * Look for item
     */
    Node* list_node;
    Node* temp_ptr;
    /*
     * First addition
     */
    //If not found, insert node
    if((list_node = List_search(vm_data)) == NULL){
        if(*is_full >= 5){
            temp_ptr = tail;
            tail->prev->next = head;
            head->prev = tail->prev;
            tail = tail->prev;
            /*
             *We have to free the memory
             */
                free(temp_ptr);
                temp_ptr = NULL;
        }
        Insert_top(vm_data);
    }
    else{ // If found, put on the top
        if(list_node == head)
            return;
        else{
            list_node->next->prev = list_node->prev;
            list_node->prev->next = list_node->next;
            if(list_node == tail)
                tail = list_node->prev;
            list_node->next = head;
            head->prev = list_node;
            list_node->prev = tail;
            head = list_node;
            tail->next = head;
            (*is_full) -= ((*is_full) >= 5 ? 0 : 1);
        }
    }
    list_node = NULL;
    temp_ptr = NULL;
    Display_frames(head);
}

/*
 * LRU Algorithm
 */
void LRU(VirtualMem* vm_data){
    static int count = 0;
    //Check if end of proper data
    //0xFFFF is not possible as the page
    //size is equal to 64 bytes, and data
    //is 'byteaddressable'
    while(vm_data->virtual_page_offset != 0xFFFF) {
        if (count < 5) {
            Renew_item(*vm_data, &count);
            count++;
            ++vm_data;
        }
        else{
            /*
             * Check if we need to put
             * vm address on top, and do it
             * if so
             */
            Renew_item(*vm_data, &count);
            ++vm_data;
        }
    }
}

/*
 * Open the file and fill the stat struct
 */
struct stat Open_file(char name[], int *file_desc){
    if((*file_desc = open(name, O_RDONLY)) == -1){
        perror("Could not open the file");
        exit(EXIT_FAILURE);
    }
    struct stat file_stats;
    if(fstat(*file_desc, &file_stats) == -1){
        perror("Could not retrieve file stats");
        exit(EXIT_FAILURE);
    }
    return file_stats;
}

/*
 * Read sys call wrapper
 */
void Read_from_file(void* data_ptr, size_t data_size, int file_desc){
    if(read(file_desc, data_ptr, data_size) == -1){
        perror("Could not read from the file");
        exit(EXIT_FAILURE);
    }
}
/*
 * I/O Redirection to the log file
 */
int Log_file(char name[]){
    int log_file_desc;
    if((log_file_desc = open(name, O_CREAT | O_WRONLY, S_IRWXU)) == -1){
        perror("Could not open the file");
        exit(EXIT_FAILURE);
    }
    /*
     * Dup the descriptors
     */
    if(dup2(log_file_desc, STDOUT_FILENO) == -1){
        perror("Cannot duplicate the descriptor");
        _exit(EXIT_FAILURE);
    }
    return log_file_desc;
}
/*
 * Display the memory
 */
void Display_the_memory(void *data_ptr, size_t data_size){
    if(write(STDOUT_FILENO, "Virtual addresses loaded: \n", (sizeof("Virtual addresses loaded: \n")-1)) == -1){
        perror("Could not write to STDOUT");
        exit(EXIT_FAILURE);
    }
    void *temp_data_ptr = (char*)data_ptr;
    for(int i = 1; i <= (data_size/ADDR_LENGTH); i++){
        if(write(STDOUT_FILENO, temp_data_ptr, ADDR_LENGTH) == -1){
            perror("Could not write to STDOUT");
            exit(EXIT_FAILURE);
        }
        if(write(STDOUT_FILENO, " ", 1) == -1){
            perror("Could not write to STDOUT");
            exit(EXIT_FAILURE);
        }
        /*
         * Break line
         */
        if((i%6)==0){
            if(write(STDOUT_FILENO, "\n", 1) == -1){
                perror("Could not write to STDOUT");
                exit(EXIT_FAILURE);
            }
        }
        temp_data_ptr+=ADDR_LENGTH;
    }
    if(write(STDOUT_FILENO, "\n", 1) == -1){
        perror("Could not write to STDOUT");
        exit(EXIT_FAILURE);
    }
}
/*
 * Retrieve the virtual page number from virtual address
 * and fill the vm_array
 */
void Fill_vm_array(void* data_ptr, VirtualMem *vm_ptr, size_t data_size){
    void *temp_data_ptr = (char*)data_ptr;
    VirtualMem *temp_vm_ptr = vm_ptr;

    char temp_str_holder[ADDR_LENGTH];
    /*
     * Clean the memory
     */
    bzero(temp_str_holder, ADDR_LENGTH);
    unsigned int temp_number;

    for(int i = 1; i <= (data_size/ADDR_LENGTH); i++) {
        strncpy(temp_str_holder, temp_data_ptr, ADDR_LENGTH);
        /*
         * strtol will automatically convert hex address number to decimal
         * I'm literally shocked you have that kind of function in C :P
         */
        temp_number = strtol(temp_str_holder, NULL, 16);
        temp_vm_ptr->virtual_page_number = temp_number>>6;
        /*
         * Double shift = big brain time
         * We assume the 32-bit int here
         * which is a possible bug-point
         */
        temp_vm_ptr->virtual_page_offset = (temp_number<<26)>>26;

        temp_vm_ptr++;
        temp_data_ptr += ADDR_LENGTH;
    }
    temp_vm_ptr->virtual_page_offset = 0xFFFF; //impossible value
    temp_vm_ptr = NULL;
    temp_data_ptr = NULL;
}

int main(){

    int file_desc, log_file_desc;
    struct stat my_stats = Open_file(ADDRESES, &file_desc);
    /*
     * Allocate enough space for the data from the file
     */
    int *data_ptr = Alloc_mem(my_stats.st_size, sizeof(char));
    VirtualMem *vm_array = Alloc_mem((my_stats.st_size/ADDR_LENGTH), VM_STRUCT_SIZE);
    /*
     * Reading from the file
     */
    Read_from_file(data_ptr, my_stats.st_size, file_desc);
    /*
     * Dup the STDOUT
     */
    log_file_desc = Log_file(LOGS);
    /*
     * Write the data array
     */
    Display_the_memory(data_ptr,my_stats.st_size);
    /*
     * Fill the vm_array
     */
    Fill_vm_array(data_ptr, vm_array, my_stats.st_size);

    /*
     * The actual start of the algorithm
     * The LRU algorithm can be implemented
     * in two ways:
     * 1. Using the PC do denote the "age" of the page (call me eminem)
     * 2. Using the stack implemented as:
     *  * Array - simple but insertion not so fast
     *  * Doubly linked list - not so simple but faster
     *
     * We will choose the 2nd option and implement the stack as doubly linked list.
     */

    VirtualMem *ptr = vm_array;
    LRU(ptr);
    /*
     * Close files
     */
    if(close(file_desc) == -1)
        perror("Could not close file");
    if(close(log_file_desc) == -1)
        perror("Could not close file");
    /*
     * Remember to free the data
     */
    Free_list(head);
    free(data_ptr);
    free(vm_array);
    return 0;
}