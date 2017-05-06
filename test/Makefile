vc4asmbinpath = ../build-Linux-armv6l-Debug

all : asm cout parser validator disasm

clean :
	rm gpu_fft_*.hex *.strip

tmp :
	-mkdir $@

.SECONDARY :

# assembler tests with hex out
asm : tmp testa_256 testa_512 testa_1k testa_2k testa_4k testa_8k testa_16k testa_32k testa_64k testa_128k testa_256k testa_512k testa_1024k testa_2048k testa_trans testa_256_new

tmp/%.strip : tmp/%.hex
	sed 's/\s*\/\/.*//' $< >$@

testa_% : tmp/gpu_fft_%.hex tmp/shader_%.strip
	diff $^ >$@

tmp/%.strip.c : tmp/%.c
	perl -pe 's"//  *(.*?)(?:\s*#.*|$$)"// \1"' $< >$@

tmp/gpu_fft_%.hex : gpu_fft_%.qasm gpu_fft.qinc gpu_fft_ex.qinc $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -V -C $@ -i vc4.qinc $<

# assembler tests with C out
cout : tmp testc_rpi_shader testh_rpi_shader

testc_% : tmp/%.strip.c tmp/%.ref.c
	diff $^ >$@

testh_% : tmp/%.h tmp/%.ref.h
	diff $^ >$@

tmp/%.c : %.qasm $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -v -c $@ -h $(subst .c,.h,$@) -i vc4.qinc $<

# disassembler tests
disasm : tmp testd_256 testd_512 testd_1k testd_2k testd_4k testd_8k testd_16k testd_32k testd_64k testd_128k testd_256k testd_512k testd_1024k testd_2048k testd_trans testd_256_new

tmp/%.dis : tmp/%.hex $(vc4asmbinpath)/vc4dis
	$(vc4asmbinpath)/vc4dis -v -x $< >$@

tmp/gpu_fft_%.hex2 : tmp/gpu_fft_%.dis $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -C $@ $<

testd_% : tmp/gpu_fft_%.hex2 tmp/gpu_fft_%.strip
	diff $^ >$@

# parser tests
parser : tmp testp.rot.hex testp.pup.hex

testp.%.hex : parser.%.qasm $(vc4asmbinpath)/vc4asm
	-$(vc4asmbinpath)/vc4asm -V -Q -c $@ -i vc4.qinc $< >tmp/parser.out 2>&1
	./checkOutput.pl $< <tmp/parser.out

# validator tests
validator : testv.VPM.hex

testv.%.hex : validator.%.qasm $(vc4asmbinpath)/vc4asm
	-$(vc4asmbinpath)/vc4asm -V -c $@ -i vc4.qinc $< >tmp/parser.out 2>&1
	./checkOutput.pl $< <tmp/parser.out

# miscellaneous
tmp/test.strip : test.hex

test : tmp/test.hex tmp/test.strip
	diff $^

tmp/test.hex : test.qasm $(vc4asmbinpath)/vc4asm
	$(vc4asmbinpath)/vc4asm -V -c $@ -i vc4.qinc $<
