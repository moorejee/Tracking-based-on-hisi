# Hisilicon Hi3516 sample Makefile
#
# include ../Makefile.param
# #ifeq ($(SAMPLE_PARAM_FILE), )
# #     SAMPLE_PARAM_FILE:=../Makefile.param
# #     include $(SAMPLE_PARAM_FILE)
# #endif
#
# # target source
# #SRC  := $(wildcard *.c)
# # SRC := sample_vdec2.c
# SRC := fhog.cpp kcftracker.cpp
# # OBJ :=　$(SRCPP:.cpp=.o)　$(SRC:%.c=%.o)
# OBJ := $(SRC:.cpp=.o)
# # OBJ := $(SRC:%.c=%.o)
#
# TARGET := $(OBJ:%.o=%)
# .PHONY : clean all
#
# all: $(TARGET)
#
#
# MPI_LIBS := $(REL_LIB)/libmpi.a
# MPI_LIBS += $(REL_LIB)/libhdmi.a
#
# LDFLAGS = -L/home/xstrive/hisi/Hi3520D_SDK_V1.0.3.2/opencv/output/lib
# DLIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_objdetect -lopencv_imgproc
#
# $(TARGET):%:%.o $(COMM_OBJ)
# 		$(CC)  $(CFLAGS) -lrt -lstdc++ -lpthread -lm -g -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(LDFLAGS) $(DLIBS)
#
# clean:
# 	@rm -f $(TARGET)
# 	@rm -f $(OBJ)
# 	@rm -f $(COMM_OBJ)
#
# cleanstream:
# #	@rm -f *.h264
# #	@rm -f *.jpg
# #	@rm -f *.mjp
# #	@rm -f *.mp4


# Hisilicon Hi3516 sample Makefile

include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif

# target source
#SRC  := $(wildcard *.c)
# SRC := sample_vdec1.c
# OBJ := $(SRC:%.c=%.o)

##sample_vdec2
SRC := sample_vdec2.c
SRCPP := fhog.cpp kcftracker.cpp
OBJ := $(SRC:%.c=%.o) $(SRCPP:%.cpp=%.o)

TARGET := $(OBJ:%.o=%)
.PHONY : clean all

all: $(TARGET)


MPI_LIBS := $(REL_LIB)/libmpi.a
MPI_LIBS += $(REL_LIB)/libhdmi.a

LDFLAGS = -L/home/xstrive/hisi/Hi3520D_SDK_V1.0.3.2/opencv/output/lib
DLIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_objdetect -lopencv_imgproc

# %.o:%.c
# 	$(CC) $(CFLAGS) -o $@  $^ $(INC_FLAGS) -lrt -lstdc++ -lpthread -lm $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(LDFLAGS) $(DLIBS)

# %.o:%.cpp
# 	$(CXX) $(CXXFLAGS) -o $@  $^ $(INC_FLAGS) -lrt -lstdc++ -lpthread -lm $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(LDFLAGS) $(DLIBS)

$(TARGET):%:$(OBJ) $(COMM_OBJ)
		$(CC)  $(CFLAGS) -lrt -lstdc++ -lpthread -lm -g -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(LDFLAGS) $(DLIBS)

# $(TARGET):%:%.o $(COMM_OBJ)
# 		$(CC)  $(CFLAGS) -lrt -lstdc++ -lpthread -lm -g -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(LDFLAGS) $(DLIBS)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(COMM_OBJ)

cleanstream:
#	@rm -f *.h264
#	@rm -f *.jpg
#	@rm -f *.mjp
#	@rm -f *.mp4
