# Make file with rules for project 2 

cacheSim : cacheSim.cpp
	g++ -g -std=c++11 -o cacheSim cacheSim.cpp
	
submit : clean # Run the clean target as a pre-req so we only  submit unbuilt files
	tar -cvzf ../proj2.tar.gz ../proj2sol
	@echo Submit ../proj2.tar.gz in Brightspace
	
	
clean : 
	-rm cacheSim 
