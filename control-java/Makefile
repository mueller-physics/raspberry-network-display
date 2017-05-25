#
# A very basic Makefile to compile the plugin
#
#

# Options for the java compiler
JFLAGS = -g -Xlint:unchecked -extdirs external 
JC = javac
JAR = jar

# remove command to clean up
RM = rm -rf

# java -> class rules

.SUFFIXES: .java .class

.java.class:  
	$(JC) $(JFLAGS) $*.java


default : org/mueller_physics/network_display/Send_Raspberry.class org/mueller_physics/network_display/UdpSender.class   

external: external/ij-1.41h.jar

external/ij-1.41h.jar:
	wget http://central.maven.org/maven2/net/imagej/ij/1.51h/ij-1.51h.jar -O external/ij-1.51h.jar
	 	

# misc rules

jar	: default
	#$(JAR) -cvf RaspberryControl_$(shell date +%y%m%d-%H%M).jar  org/mueller_physics/*/*.class org/mueller_physics/*/*.java plugins.config
	$(JAR) -cvf RaspberryControl_$(shell git rev-parse HEAD | head -c 10).jar  org/mueller_physics/*/*.class org/mueller_physics/*/*.java plugins.config


doc:	doc/index.html

doc/index.html : $(wildcard bbp/*/*.java) $(wildcard bbp/*/*/*.java)
	javadoc -d doc/  -subpackages org.mueller_physics
	touch doc/index.html

clean :
	$(RM) org/mueller_physics/*/*.class *.jar doc/*
