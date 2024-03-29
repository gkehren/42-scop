# ------------------
CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lGL -lglfw
INCDIR = -I include/ -I src/imgui/
# ==================

# ------ Path ------
SRCDIR = src
OBJDIR = obj
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
SRC = $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/imgui/*.cpp $(SRCDIR)/glad.c)
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
GLAD_SRC = $(SRCDIR)/glad.c
# ==================

TARGET = scop

all: ${TARGET}

${TARGET}: ${OBJ}
	@echo ${CYAN} " - Compiling $@" $(RED)
	@${CXX} -o $@ $^ ${LDFLAGS} ${INCDIR}
	@echo $(GREEN) " - OK" $(EOC)

${OBJDIR}/%.o: ${SRCDIR}/%.cpp
	@mkdir -p $(@D)
	@echo ${PURPLE} " - Compiling $< into $@" ${EOC}
	@${CXX} ${CXXFLAGS} ${INCDIR} -c -o $@ $<

%.cpp:
	@echo ${RED}"Missing file : $@" ${EOC}

clean:
	@rm -rf ${OBJDIR}

fclean:	clean
	@rm -f ${TARGET}

re:	fclean
	@${MAKE} all

.PHONY:	all clean fclean re
