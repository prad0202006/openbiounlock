CC ?= cc
CFLAGS ?= -O2 -fPIC -Wall -Wextra -Werror
PAM_MODULE ?= pam_openbiounlock.so

all: $(PAM_MODULE)

$(PAM_MODULE): pam_openbiounlock.c
	$(CC) $(CFLAGS) -shared -o $@ $< -lpam

clean:
	rm -f $(PAM_MODULE)
