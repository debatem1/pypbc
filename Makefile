BUILD_DIR = build/lib.*
CLEAN_BUILD = if [ -d build ]; then rm -r build; fi;
CLEAN_DIST = if [ -d dist ]; then rm -r dist; fi; if [ -f MANIFEST ]; then rm MANIFEST; fi;
CLEAN = $(CLEAN_BUILD) $(CLEAN_DIST)
BUILD = $(CLEAN) ./setup.py build
TEST = test

pypbc:
	$(BUILD)
	
dist:
	$(BUILD) sdist
	$(CLEAN)

play:
	$(BUILD)
	cd $(BUILD_DIR); python3 -i -c "from pypbc import *; from KSW import *"
	$(CLEAN)

test:
	$(BUILD)
	chmod +x $(BUILD_DIR)/test.py
	$(BUILD_DIR)/test.py -v
	python3 $(BUILD_DIR)/KSW.py
	$(CLEAN)

commit:
	$(MAKE) $(TEST)
	rm *~ 2> /dev/null
	git add .
	git commit
	git push origin master

install:
	$(BUILD) install
	$(MAKE) $(TEST)
	$(CLEAN)
	
clean:
	$(CLEAN)
