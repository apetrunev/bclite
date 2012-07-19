GROFF=groff
OUTPUT=presentation.ps

all: presentation.ps 

presentation.ps: presentation.ms
	iconv -f utf-8 -t koi8-r $< | preconv -e KOI8-R  \
	| $(GROFF) -p -Tps -P-pa5 -P-l -ms -ru > $@

clean:
	rm $(OUTPUT) 
