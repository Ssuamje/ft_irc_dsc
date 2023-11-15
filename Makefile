CXX = c++
CXXFLAGS = -std=c++98 -I./include -I./include/utils
RM = rm -rf
SRC = main ./source/ServerKqueue ./source/Client ./source/Channel \
	  ./source/utils/utils ./source/utils/Buffer ./source/utils/CommandExecute \
	  ./source/utils/error ./source/utils/Message ./source/utils/Print \
	  ./source/utils/reply
SRCC = $(addsuffix .cpp, $(SRC))
OBJ = $(addsuffix .o, $(SRC))
NAME = ircserv
ifdef DEBUG
	CXXFLAGS += -fsanitize=address -DDEBUG
endif

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $<

clean:
	$(RM) $(OBJ)

fclean:
	make -s clean
	$(RM) $(NAME)

re:
	make -s fclean
	make -s all

.PHONY: all clean fclean re
