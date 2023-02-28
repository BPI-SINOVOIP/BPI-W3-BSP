#!/bin/bash

dirclean()
{
	make -f buildroot/Makefile \
		dbserver-dirclean \
		common_algorithm-dirclean \
		ipcweb-backend-dirclean \
		libgdbus-dirclean \
		libIPCProtocol-dirclean \
		librkdb-dirclean \
		mediaserver-dirclean \
		camera_engine_rkaiq-dirclean \
		netserver-dirclean \
		storage_manager-dirclean \
		rkmedia-dirclean \
		rk_oem-dirclean \
		mpp-dirclean \
		ipc-daemon-dirclean \
		rockface-dirclean \
		CallFunIpc-dirclean \
		isp2-ipc-dirclean \
###
}

sync_mod()
{
	.repo/repo/repo sync -c --no-tags \
		app/dbserver \
		app/ipcweb-backend \
		app/libgdbus \
		app/libIPCProtocol \
		app/librkdb \
		app/mediaserver \
		app/netserver \
		app/ipc-daemon \
		app/storage_manager \
		external/camera_engine_rkaiq \
		external/rkmedia \
		external/common_algorithm \
		external/rockface \
		external/mpp \
		external/CallFunIpc \
		external/isp2-ipc \
###
}

rebuild()
{
	make -f buildroot/Makefile \
		dbserver-rebuild \
		common_algorithm-rebuild \
		libgdbus-rebuild \
		libIPCProtocol-rebuild \
		librkdb-rebuild \
		CallFunIpc-rebuild \
		camera_engine_rkaiq-rebuild \
		isp2-ipc-rebuild \
		ipcweb-backend-rebuild \
		netserver-rebuild \
		storage_manager-rebuild \
		rk_oem-rebuild \
		mpp-rebuild \
		ipc-daemon-rebuild \
		rockface-rebuild \
		rkmedia-rebuild \
		mediaserver-rebuild \
###
}

unset NEW_OPTS
for option in ${OPTIONS}; do
        echo "processing board option: $option"
        case $option in
		# handle board commands
		app-clean|app-rebuild|app-sync)
			source buildroot/build/envsetup.sh $RK_CFG_BUILDROOT
			eval ${option/-/_}
			exit 0
			;;
                *)
                        NEW_OPTS="$NEW_OPTS $option"
                        ;;
        esac
done
export OPTIONS=$NEW_OPTS
