from memory_lib import *
from command_lib import *
from tkinter import *


def run_command(name_of_program,command_list,pc,window_output):
    
    #check for typo
    if (command_list[0] not in list_of_commands):
        print("Wrong Command at line: "+str(pc+1))
        quit()

    command_list = check_array_index(name_of_program,command_list)
    
    if (command_list[0]=='ADD'):
        var_name = command_list[1]
        var1= var_or_value(name_of_program,command_list[2])
        var2 = var_or_value(name_of_program,command_list[3])
        ADD(var_name,var1,var2,name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif (command_list[0]=='SUB'):
        var_name = command_list[1]
        var1= var_or_value(name_of_program,command_list[2])
        var2 = var_or_value(name_of_program,command_list[3])
        SUB(var_name,var1,var2,name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif (command_list[0]=='MUL'):
        var_name = command_list[1]
        var1= var_or_value(name_of_program,command_list[2])
        var2 = var_or_value(name_of_program,command_list[3])
        MUL(var_name,var1,var2,name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif (command_list[0]=='DIV'):
        var_name = command_list[1]
        var1= var_or_value(name_of_program,command_list[2])
        var2 = var_or_value(name_of_program,command_list[3])
        DIV(var_name,var1,var2,name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif (command_list[0]=='MOD'):
        var_name = command_list[1]
        var1= var_or_value(name_of_program,command_list[2])
        var2 = var_or_value(name_of_program,command_list[3])
        MOD(var_name,var1,var2,name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='SET'):
        var_name= command_list[1]
        var = var_or_value(name_of_program,command_list[2])
        SET(var_name,var,name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='BRGT'):
        var1 = var_or_value(name_of_program,command_list[1])
        var2 = var_or_value(name_of_program,command_list[2])
        label = memory[name_of_program][command_list[3]]
        pc=BRGT(var1,var2,label,pc,name_of_program)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='BRGE'):
        var1 = var_or_value(name_of_program,command_list[1])
        var2 = var_or_value(name_of_program,command_list[2])
        label = memory[name_of_program][command_list[3]]
        pc=BRGE(var1,var2,label,pc,name_of_program)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='BRLT'):
        var1 = var_or_value(name_of_program,command_list[1])
        var2 = var_or_value(name_of_program,command_list[2])
        label = memory[name_of_program][command_list[3]]
        pc=BRLT(var1,var2,label,pc,name_of_program)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='BRLE'):
        var1 = var_or_value(name_of_program,command_list[1])
        var2 = var_or_value(name_of_program,command_list[2])
        label = memory[name_of_program][command_list[3]]
        pc=BRLE(var1,var2,label,pc,name_of_program)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='BREQ'):
        var1 = var_or_value(name_of_program,command_list[1])
        var2 = var_or_value(name_of_program,command_list[2])
        label = memory[name_of_program][command_list[3]]
        pc=BREQ(var1,var2,label,pc,name_of_program)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='BRA'):
        label = memory[name_of_program][command_list[1]]
        pc=BRA(label,name_of_program)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='SLEEP'):
        var = var_or_value(name_of_program,command_list[1])
        SLEEP(var)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='PRINT'):
        PRINT(name_of_program,command_list,window_output)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='LOAD'):
        LOAD(command_list[1],command_list[2],name_of_program)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='STORE'):
        var = var_or_value(name_of_program,command_list[2])
        STORE(command_list[1],var)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='DOWN'):
        sem_value = read_from_global_mem(command_list[1])
        pc = DOWN(command_list[1],sem_value,pc)
        insert_to_mem(name_of_program,'pc',pc)
    elif(command_list[0]=='UP'):
        sem_value = read_from_global_mem(command_list[1])
        UP(command_list[1],sem_value)
        insert_to_mem(name_of_program,'pc',pc+1)
    elif(command_list[0]=='RETURN'):
        RETURN(name_of_program)

def insert_arguments(args,p_id):

    argc=len(args)
    SET('$argc',argc,p_id)
    SET('$argv[0]',p_id,p_id)

    for i in range(1,argc):
        argv = '$argv'+'['+str(i)+']'
        SET(argv,args[i],p_id)
