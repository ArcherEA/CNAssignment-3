#ifndef __DESCRIPTOR_H
#define __DESCRIPTOR_H
void add_trailing_spaces(char *dest, int size, int num_of_spaces);
int initiate_descriptor();
int write_descriptor(int pid, char file_name[80],int file_desc,int deldes=0);
int read_descriptor();
int check_descriptor(char file_name[80]);
int delete_descriptor(int file_desc);
int clear_descriptor();
#endif /* __DESCRIPTOR_H */
