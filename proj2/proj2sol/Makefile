# Make file with rules for project 2 

cacheSim : 
	@echo This rule should build the cacheSim executable file
	
submit : clean # Run the clean target as a pre-req so we only  submit unbuilt files
	tar -cvzf ../proj2.tar.gz ../proj2sol
	@echo Submit ../proj2.tar.gz in Brightspace
	
	
clean : 
	-rm cacheSim # Add any other files this Makefile created
