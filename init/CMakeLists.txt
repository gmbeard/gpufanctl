configure_file(
    runit/run.in
    sv/gpufanctl/run
    @ONLY
)

configure_file(
    systemd/gpufanctl.service.in
    systemd/user/gpufanctl.service
    @ONLY
)

include(GNUInstallDirs)

# Installs the runit service. Not installed by default. Requires...
#
# ```
# $ cmake --install . --component gpufanctl-runit
# ```
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/sv/gpufanctl/run"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/sv/gpufanctl"
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    COMPONENT gpufanctl-runit
    EXCLUDE_FROM_ALL
)

# Installs the systemd unit file. Not installed by default. Requires...
#
# ```
# $ cmake --install . --component gpufanctl-systemd
# ```
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/systemd/user/gpufanctl.service"
    DESTINATION "lib/systemd/user/gpufanctl.service"
    PERMISSIONS
        OWNER_READ OWNER_WRITE
        GROUP_READ
        WORLD_READ
    COMPONENT gpufanctl-systemd
    EXCLUDE_FROM_ALL
)
