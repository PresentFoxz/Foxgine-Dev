.PHONY: pc gba clean

pc:
	$(MAKE) -f Makefile_win $(filter-out $@,$(MAKECMDGOALS))

gba:
	$(MAKE) -f Makefile_gba $(filter-out $@,$(MAKECMDGOALS))

clean:
	$(MAKE) -f Makefile_win clean
	$(MAKE) -f Makefile_gba clean