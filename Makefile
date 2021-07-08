CC			=	gcc -std=gnu11 -O0 -g3 -ggdb #-fsanitize=address
CPPFLAGS	=	
CFLAGS		=	-W -Wall -Wextra -Werror
LDFLAGS		=	

SRC		=	$(wildcard src/*.c)
OBJ		=	$(SRC:.c=.o)

NAME		=	clisp
TARGET		=	./$(NAME).exe

all			:	$(TARGET)

$(TARGET)	:	$(OBJ)
			$(CC) -o $@ $^ $(LDFLAGS)

clean		:
			$(RM) $(OBJ)

fclean		:	clean
			$(RM) $(TARGET)

re		:	fclean all

.PHONY		:	all clean fclean re