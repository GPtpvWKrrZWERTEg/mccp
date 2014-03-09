ifeq ($(__DEPLIBS__),.vars.)

MCCP_LIB	= libmccp.la
MCCP_LIB_DIR	= $(BUILD_SRCDIR)/lib
DEP_MCCP_LIB	= $(MCCP_LIB_DIR)/$(MCCP_LIB)

endif

ifeq ($(__DEPLIBS__),.rules.)

$(DEP_MCCP_LIB)::
	(cd $(MCCP_LIB_DIR) && $(MAKE) $(MCCP_LIB))

endif

