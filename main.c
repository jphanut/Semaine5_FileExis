#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

/*
 * @pre filename != NULL
 * @post return 0 if the file {filename} exist. -1 Otherwise.
 */
 int file_exists(char *filename) {
    if ( filename == NULL ) return -1;
    int f = open(filename, O_RDWR);
    if (f == -1) return -1;
    close(f);
    return 0;
 }

 typedef struct point {
    int x;
    int y;
    int z;
} point_t;

/*
 * @pre pt != NULL, pointer to the first point_t in the array
 *      size > 0, the length of the array.
 *      filename != NULL
 * @post writes the array of point_t in the file.
 *       return 0 is everything worked correctly
 *       -1 if open() failed.
 *       -2 if close() failed.
 *       -3 if mmap() failed.
 *       -4 if munmap() failed.
 *       -5 if msync() failed.
 *       -6 if ftruncate() failed.
 *
 */
int save(point_t *pt, int size, char *filename) {
    int f = open(filename, O_RDWR | O_CREAT, (mode_t)0600);
    if (f == -1) return -1;
    int memsize = size*sizeof(point_t);
    int rc = ftruncate(f, memsize);
    if ( rc == -1) return -6;
    point_t *ptr_map = mmap(NULL,memsize,PROT_READ|PROT_WRITE,MAP_SHARED, f,0);
    if(ptr_map == MAP_FAILED){
        return -3;
    }
    memcpy(ptr_map, pt, memsize);
    rc = msync(ptr_map, memsize, MS_SYNC);
    if ( rc == -1) return -5;
    rc = munmap(ptr_map, memsize);
    if ( rc == -1) return -4;
    rc = close(f);
    if ( rc == -1) return -2;
    return 0;
}

/*
 * @pre filename != NULL, name of the file
 * @post returns the sum of all integers stored in the binary file.
 *       return -1 if the file can not be open.
 *       return -2 if the file can not be closed.
 *       return -3 if mmap() fails.
 *       return -4 if munmap() fails.
 *       return -5 if fstat() fails.
 */
int sum_file(char *filename) {
    int f = open(filename, O_RDONLY);
    if (f == -1) return -1;
    struct stat s;
    int rc = fstat (f, &s);
    if ( rc == -1) return -5;
    int memsize = s.st_size;
    if ( memsize == 0 ) {
        rc = close(f);
        if ( rc == -1) return -2;
        return 0;
    }
    int nbrelem = memsize/sizeof(int);
    int sumelems = 0;
    int * arr_int = mmap(NULL,memsize, PROT_READ, MAP_PRIVATE, f, 0);
    if(arr_int == MAP_FAILED){
        return -3;
    }
    for (int i = 0; i < nbrelem; i++) {
        sumelems += arr_int[i];
    }
    rc = munmap(arr_int, memsize);
    if ( rc == -1) return -4;
    rc = close(f);
    if ( rc == -1) return -2;
    return sumelems;
}

/*
 * @pre filename != NULL, index >= 0
 * @post return the integer at the index {index}
 *       of the array in the file {filename}.
 *       return -1 in case of error.
 *       return -2 if index >= length of array.
 */
int get(char *filename, int index) {
    int f = open(filename, O_RDONLY);
    if (f == -1) return -1;
    struct stat s;
    int rc = fstat (f, &s);
    if ( rc == -1) return -1;
    int memsize = s.st_size;
    if ( memsize == 0 ) {
        rc = close(f);
        return -1;
    }
    int nbrelem = memsize/sizeof(int);
    if ( index >= nbrelem) {
       rc = close(f);
       return -2;
    }
    int * arr_int = mmap(NULL, (index+1)*sizeof(int), PROT_READ, MAP_PRIVATE, f, 0);
    if(arr_int == MAP_FAILED){
        return -1;
    }
    int elem = arr_int[index];
    rc = munmap(arr_int, memsize);
    if ( rc == -1) return -1;
    rc = close(f);
    if ( rc == -1) return -1;
    return elem;
}

/*
 * @pre filename != NULL, index >= 0
 * @post set the element in the file {filename}
 *       at index {index} at value {value}.
 *       do nothing in case of errors
 */
void set(char *filename, int index, int value) {
    int f = open(filename, O_RDWR, (mode_t)0600);
    struct stat s;
    int rc = fstat (f, &s);
    if ( rc == -1) {
        close(f);
        return;
    }
    int memsize = s.st_size;
    int * arr_int = mmap(NULL,memsize,PROT_READ|PROT_WRITE,MAP_SHARED, f,0);
    if(arr_int == MAP_FAILED){
        close(f);
        return;
    }
    arr_int[index] = value;
    rc = msync(arr_int, memsize, MS_SYNC);
    if ( rc == -1) {
        close(f);
        return;
    }
    rc = munmap(arr_int, memsize);
    if ( rc == -1) {
        close(f);
        return;
    }
    close(f);
}

/*
 * @pre file_name != NULL, name of the original file
 *      new_file_name != NULL, name of the new file (the copy)
 *
 * @post copy the contents of {file_name} to {new_file_name}.
 *       return 0 if the function terminates with success, -1 in case of errors.
 */
int copy(char *file_name, char *new_file_name) {
    int f = open(file_name, O_RDONLY);
    if (f == -1) return -1;


    struct stat s;
    int rc = fstat (f, &s);
    if ( rc == -1) return -1;
    int memsize = s.st_size;

    int f_copy = open(new_file_name, O_RDWR | O_CREAT, s.st_mode);
    if (f_copy == -1) {
        close(f);
        return -1;
    }
    rc = ftruncate(f_copy, memsize);
    if (rc == -1) {
        close(f);
        close(f_copy);
        return -1;
    }

    char * buffer = mmap(NULL, memsize , PROT_READ, MAP_PRIVATE, f, 0);
    char * buffer_copy = mmap(NULL,memsize,PROT_READ|PROT_WRITE,MAP_SHARED, f_copy,0);

    if(buffer == MAP_FAILED || buffer_copy == MAP_FAILED){
        close(f);
        close(f_copy);
        return -1;
    }

    memcpy(buffer_copy, buffer, memsize);
    rc = msync(buffer_copy, memsize, MS_SYNC);
    if ( rc == -1) {
        close(f);
        close(f_copy);
        return -1;
    }

    rc = munmap(buffer, memsize);
    if ( rc == -1) {
        close(f);
        close(f_copy);
        return -1;
    }
    rc = munmap(buffer_copy, memsize);
    if ( rc == -1) {
        close(f);
        close(f_copy);
        return -1;
    }
    rc = close(f);
    if ( rc == -1) return -1;
    rc = close(f_copy);
    if ( rc == -1) return -1;
    return 0;
}

int main()
{
    printf("Exercice 1: Semaine 5 - File exists\n");
    printf("***********************************\n");
    char * filepath = "/home/margauxhanut/ProjectC/Semaine5_FileExist/testjp";
    printf("filepath=%s\n", filepath);
    if ( file_exists(filepath) == -1 ) printf("The file does not exist\n");
    else printf("The file exists\n");
    char * filepath2 = "/home/margauxhanut/ProjectC/Semaine5_FileExist/testjp2";
    printf("filepath=%s\n", filepath2);
    if ( file_exists(filepath2) == -1 ) printf("The file does not exist\n");
    else printf("The file exists\n");

    printf("Exercice 2: Semaine 5 - Save struct into file\n");
    printf("*********************************************\n");
    int size = 15;
    point_t * arr_points =  (point_t *)malloc(size*sizeof(point_t));
    for ( int i=1; i <= 5; i++)
    {
        point_t * current = arr_points+i-1;
        current->x = i;
        current->y = 0;
        current->z = 0;
        current = arr_points+i+4;
        current->x = i;
        current->y = i;
        current->z = 0;
        current = arr_points+i+9;
        current->x = i;
        current->y = i;
        current->z = i;
    }
    int rc = save(arr_points, size, filepath);
    printf ("Return code RC=%d\n", rc);
    if ( rc == 0 ) printf ("Save file succeeded\n");
    free(arr_points);

    printf("Exercice 3: Semaine 5 - Reading integers in a binary file\n");
    printf("*********************************************************\n");
    rc = sum_file(filepath);
    printf ("Return code RC=%d\n", rc);
    if ( rc >= 0 ) printf ("The sum of all integers stored in the file is %d\n", rc);

    printf("Exercice 4: Semaine 5 - Get and set on array stored in binary file\n");
    printf("******************************************************************\n");
    rc = get(filepath, 44);
    printf ("Return code RC=%d\n", rc);
    if ( rc >= 0 ) printf ("The integer at the index 44 is %d\n", rc);
    rc = get(filepath, 45);
    printf ("Return code RC=%d\n", rc);
    if ( rc == -2 ) printf ("The index is out of bound\n");

    printf ("Set the integer at the index 44 to the value of 6\n");
    set(filepath, 44, 6);
    rc = get(filepath, 44);
    printf ("Return code RC=%d\n", rc);
    if ( rc >= 0 ) printf ("The integer at the index 44 is %d\n", rc);
        printf ("Set the integer at the index 44 to the value of 5\n");
    set(filepath, 44, 5);
    rc = get(filepath, 44);
    printf ("Return code RC=%d\n", rc);
    if ( rc >= 0 ) printf ("The integer at the index 44 is %d\n", rc);


    printf("Exercice 5: Semaine 5 - File copy\n");
    printf("*********************************\n");
    char * filepath_copy = "/home/margauxhanut/ProjectC/Semaine5_FileExist/testjp_copy";
    rc = copy(filepath, filepath_copy);
    printf ("Return code RC=%d\n", rc);
    if ( file_exists(filepath_copy) == -1 ) printf("The file copy does not exist\n");
    else printf("The file copy exists\n");
    return 0;
}
