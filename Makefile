# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gkehren <gkehren@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/07/30 22:16:55 by gkehren           #+#    #+#              #
#    Updated: 2023/08/05 19:11:37 by gkehren          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

FILES:= main Scop
NAME:= scop

# ------------------
CC:=c++
SRCPATH:=src/
INCLUDES:= -I includes/ -I imgui/
CCHPATH:=obj/
CFLAGS:=-lGLEW -lGL -lglfw
# ==================

# ----- Colors -----
BLACK:="\033[1;30m"
RED:="\033[1;31m"
GREEN:="\033[1;32m"
CYAN:="\033[1;35m"
PURPLE:="\033[1;36m"
WHITE:="\033[1;37m"
EOC:="\033[0;0m"
# ==================

# ------ Auto ------
SRC:=$(addprefix $(SRCPATH),$(addsuffix .cpp,$(FILES)))
OBJ:=$(addprefix $(CCHPATH),$(addsuffix .o,$(FILES)))
SRC+= $(wildcard imgui/*.cpp)
# ==================
CCHF:=.cache_exists

all: ${NAME}

${NAME}: ${OBJ}
	@echo ${CYAN} " - Compiling $@" $(RED)
	@${CC} ${CFLAGS} ${SRC} -o ${NAME}
	@echo $(GREEN) " - OK" $(EOC)

${CCHPATH}%.o: ${SRCPATH}%.cpp
	@mkdir -p $(@D)
	@echo ${PURPLE} " - Compiling $< into $@" ${EOC}
	@${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

%.cpp:
	@echo ${RED}"Missing file : $@" ${EOC}

clean:
	@rm -rf ${CCHPATH}

fclean:	clean
	@rm -f ${NAME}
	@rm -rf ${NAME}.dSYM/

re:	fclean
	@${MAKE} all

.PHONY:	all clean fclean re