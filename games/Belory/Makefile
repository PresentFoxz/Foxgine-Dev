.PHONY: pc playdate clean

pc:
	$(MAKE) -f Makefile_win $(filter-out $@,$(MAKECMDGOALS))

playdate:
	$(MAKE) -f Makefile_playdate $(filter-out $@,$(MAKECMDGOALS))

clean:
	$(MAKE) -f Makefile_win clean
	$(MAKE) -f Makefile_playdate clean

# Swallow forwarded goals
%:
	@: