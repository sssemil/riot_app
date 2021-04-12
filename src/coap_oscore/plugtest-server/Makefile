USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gcoap

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

CFLAGS += -DOSCORE_NANOCOAP_MEMMOVE_MODE

# The +1500 it's still a but much for my taste, and I should confirm my suspicion about
# most of it being the crypto library at some point.
CFLAGS += -DGCOAP_STACK_SIZE="THREAD_STACKSIZE_DEFAULT + DEBUG_EXTRA_STACKSIZE + sizeof(coap_pkt_t) + 1500"
# As the main thread sends messages, it needs an increased size.
CFLAGS += -DTHREAD_STACKSIZE_MAIN="THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF + 1500"

# Make sanely sized blocks
CFLAGS += -DGCOAP_PDU_BUF_SIZE=600

APPLICATION ?= test_$(notdir $(subst -,_,$(patsubst %/,%,$(CURDIR))))

RIOTBASE ?= $(CURDIR)/../../../RIOT

BOARD ?= native

DEVELHELP ?= 1

# C11 features are accepted in the OSCORE implementation, but that causes
# pedantic Werrors to show up in RIOT source; gnu11 seems to ignore those.
CFLAGS += -std=gnu11

EXTERNAL_MODULE_DIRS += $(abspath ../../../)/liboscore/backends/riot-module
USEMODULE += oscore

# Pick a backend; without, no algorithms are available
USEMODULE += libcose_crypt_hacl
# That would offer AES-CCM algorithms, but using it requires changing the RIOT
# package to point to the branch at https://github.com/bergzand/libcose/pull/91
# until that is completed and a version including it has been mada available in
# RIOT.
# USEMODULE += libcose_crypt_tinydtls

#EXTERNAL_MODULE_DIRS += $(abspath ..)/testwrapper

#TESTS = ../test-evaluator.sh

# workarounds for https://github.com/RIOT-OS/RIOT/issues/13121
USEMODULE += gnrc_sock
USEMODULE += gnrc_ipv6

include $(RIOTBASE)/Makefile.include

