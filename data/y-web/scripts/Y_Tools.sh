#!/bin/sh
# -----------------------------------------------------------
# Tools (yjogol)
# -----------------------------------------------------------

. ./_Y_Globals.sh
. ./_Y_Library.sh

# ===========================================================
# Settings : Styles
# ===========================================================
# -----------------------------------------------------------
# Style List
# -----------------------------------------------------------
style_get()
{
	check_Y_Web_conf
	active_style=$(config_get_value_direct $y_config_Y_Web 'style')

	y_path_directory=$(config_get_value_direct $y_config_nhttpd 'WebsiteMain.directory')
	y_path_override_directory=$(config_get_value_direct $y_config_nhttpd 'WebsiteMain.override_directory')

	style_list=""
	style_list="$style_list $(find $y_path_override_directory/styles -name 'Y_Dist-*')"
	style_list="$style_list $(find $y_path_directory/styles -name 'Y_Dist-*')"

	f_list=""
	html_option_list=""
	for f in $style_list
	do
		echo $f_list | grep ${f##*/}
		if [ $? == 0 ]; then
			continue
		fi
		f_list="$f_list ${f##*/}"

		style=$(echo "$f" | sed -e s/^.*Y_Dist-//g | sed -e s/.css//g)
		if [ "$style" = "$active_style" ]
		then
			sel="selected='selected'"
		else
			sel=""
		fi
		opt="<option value='$style' $sel>${style//_/ }</option>"
		html_option_list="$html_option_list $opt"
	done
	echo "$html_option_list"
}

# -----------------------------------------------------------
# Set Style: override Y_Main.css   $1=Style-Name
# -----------------------------------------------------------
style_set()
{
	# This function should be called one time after installing a new image
	# to get sure, the right skin is installed too

	style=${1:-$(config_get_value_direct $y_config_Y_Web 'style')}
	test -n "$style" || return

	y_path_directory=$(config_get_value_direct $y_config_nhttpd 'WebsiteMain.directory')
	y_path_override_directory=$(config_get_value_direct $y_config_nhttpd 'WebsiteMain.override_directory')

	cd $y_path_directory
	if [ -e $y_path_override_directory/styles/Y_Dist-$style.css ]; then
		cp $y_path_override_directory/styles/Y_Dist-$style.css Y_Dist.css
	elif [ -e $y_path_directory/styles/Y_Dist-$style.css ]; then
		cp $y_path_directory/styles/Y_Dist-$style.css Y_Dist.css
	else
		config_set_value_direct $y_config_Y_Web 'style'
	fi
}

# -----------------------------------------------------------
# Image Backup - build form
# -----------------------------------------------------------
image_upload()
{
	if [ -s "$y_upload_file" ]
	then
		msg="<b>Image upload ok</b><br>"
		msg="$msg <script language='JavaScript' type='text/javascript'>window.setTimeout('parent.do_image_upload_ready()',1000)</script>"
	else
		msg="Upload-Problem.<br>Please, try again."
		msg="$msg <script language='JavaScript' type='text/javascript'>window.setTimeout('parent.do_image_upload_ready_error()',1000)</script>"
	fi
	y_format_message_html
}

# -----------------------------------------------------------
# copies Uploadfile to $1
# -----------------------------------------------------------
upload_copy()
{
	if [ -s "$y_upload_file" ]
	then
		cp "$y_upload_file" "$1"
	else
		msg="Upload-Problem.<br>Try again, please."
	fi
}

# -----------------------------------------------------------
zapit_upload()
{
	msg="$1 hochgeladen<br><a href='/Y_Settings_zapit.yhtm'><u>next file</u></a>"
	upload_copy "$y_path_zapit/$1"
	y_format_message_html
}

# -----------------------------------------------------------
# Mount from Neutrino-Settings $1=nr
# -----------------------------------------------------------
do_mount()
{
	config_open $y_config_neutrino
	fstype=`config_get_value "network_nfs_type_$1"`
	ip=`config_get_value "network_nfs_ip_$1"`
	local_dir=`config_get_value "network_nfs_local_dir_$1"`
	dir=`config_get_value "network_nfs_dir_$1"`
	options1=`config_get_value "network_nfs_mount_options1_$1"`
	options2=`config_get_value "network_nfs_mount_options2_$1"`
	username=`config_get_value "network_nfs_username_$1"`
	password=`config_get_value "network_nfs_password_$1"`

	# check options
	if [ "$options1" = "" ]
	then
		options1=options2
		options2=""
	fi

	# default options
	if [ "$options1" = "" -a "$options2" = "" ]
	then
		if [ "$fstype" = "0" ] # nfs
		then
			options1="soft"
			options2="nolock"
		elif [ "$fstype" = "1" ] # cifs
		then
			options1="ro"
			options2=""
		elif [ "$fstype" = "2" ] # lufs
		then
			options1=""
			options2=""
		fi
	fi

	# build mount command
	case "$fstype" in
		0) # nfs
			cmd="mount -t nfs $ip:$dir $local_dir"
		;;
		1) # cifs
			cmd="mount -t cifs //$ip/$dir $local_dir -o username=$username,password=$password";
		;;
		2) # lufs
			cmd="lufsd none $local_dir -o fs=ftpfs,username=$username,password=$password,host=$ip,root=/$dir";
		;;
		default)
			echo "mount type not supported"
	esac

	if [ "$options1" != "" ]
	then
		if [ "$fstype" = "0" ] # nfs
		then
			cmd="$cmd -o $options1"
		else
			cmd="$cmd,$options1"
		fi
	fi

	if [ "$options2" != "" ]
	then
		cmd="$cmd,$options2"
	fi

	res=`$cmd`
	echo "$cmd" >/tmp/mount.log
	echo "$res" >>/tmp/mount.log
	echo "$res"
	echo "view mounts"
	m=`mount`
	msg="mount cmd:$cmd<br><br>res=$res<br>view Mounts;<br>$m"
	y_format_message_html
}

# -----------------------------------------------------------
# unmount $1=local_dir
# -----------------------------------------------------------
do_unmount()
{
	umount $1
}

# -----------------------------------------------------------
# AutoMount
# deactivate mount "#" replaces "---" and "=" replaced ",,"
# -----------------------------------------------------------
do_automount_list()
{
	i="1"
	sel="checked='checked'"
	cat $1|sed /#/d|sed -n /-fstype/p|\
	while read mountname options share
	do
		mountvalue=`echo "$mountname"|sed -e "s/#/---/g"`
		echo "<input type='radio' name='R1' value='$mountvalue' $sel/>$i: $mountname ($share)<br/>"
		sel=""
		i=`expr $i + 1`
	done
}

# -----------------------------------------------------------
do_automount_getline()
{
	mountname=`echo "$2"|sed -e "s/---/#/g"`
	cat $1 | sed -n "/^[#]*$mountname[^a-zA-Z0-9]/p"
}

# -----------------------------------------------------------
# $1:filename, $2:mountname, $3-*:mountstring
# -----------------------------------------------------------
do_automount_setline()
{
	if ! [ -e $1 ]; then
		cp /var/etc/auto.net $1
	fi
	filename=$1
	mountname=`echo "$2"|sed -e "s;---;;g"`
	shift 2
	mountset=`echo "$*"|sed -e "s;---;#;g" -e "s;,,;=;g"`
	cp $filename "$filename.old"
	chmod ou+rwss $filename
	ex=`cat $filename|sed -n "/^$mountname[^a-zA-Z0-9].*$/p"`
	if [ "$ex" = "" ]; then
		echo "$mountset" >>$filename
	else
		tmp=`cat "$filename"|sed -e "s;^$mountname[^a-zA-Z0-9].*$;$mountset;g"`
	echo "$tmp" >$filename
	fi

	kill -HUP `cat /var/run/automount.pid`
}

# -----------------------------------------------------------
# Execute shell command
# 1: directory 2: append [true|false] 3+: cmd
# -----------------------------------------------------------
do_cmd()
{
	cd $1
	pw1="$1"
	app="$2"
	shift 2

	if [ "$1" = "cd" ]
	then
		cd $2
	else
		tmp=`$*` #Execute command
	fi
	pw=`pwd`
	echo '<html><body><form name="o"><textarea name="ot">'
	echo "$pw1>$*"
	echo "$tmp"
	echo '</textarea></form>'
	echo '<script language="JavaScript" type="text/javascript">'
	if [ "$app" = "true" ]
	then
		echo 'parent.document.co.cmds.value += document.o.ot.value;'
	else
		echo 'parent.document.co.cmds.value = document.o.ot.value;'
	fi
	echo "parent.set_pwd('$pw');"
	echo 'parent.setCaretToEnd(parent.document.co.cmds);'
	echo 'parent.document.f.cmd.focus();'
	echo '</script></body></html>'
}

# -----------------------------------------------------------
# yInstaller
# un-tar uploaded file to /tmp. Execute included install.sh
# -----------------------------------------------------------
do_installer()
{
	# clean up
	if [ -s "$y_out_html" ]
	then
		rm $y_out_html
	fi

	if [ -s "$y_upload_file" ]
	then
		# unpack /tmp/upload.tmp
		cd $y_path_tmp
		tar -xf "$y_upload_file"
		rm $y_upload_file
		if [ -s "$y_install" ] #look for install.sh
		then
			chmod 755 $y_install
			o=`$y_install` # execute
			rm -f $y_install # clean up
			if [ -s "$y_out_html" ] #html - output?
			then
				echo '<html><head>'
				echo '<link rel="stylesheet" type="text/css" href="/Y_Main.css">'
				echo '<link rel="stylesheet" type="text/css" href="/Y_Dist.css">'
				echo '<link rel="stylesheet" type="text/css" href="/Y_User.css">'
				echo "<meta http-equiv='refresh' content='0; $y_out_html'>"
				echo '</head>'
				echo "<body><a href='$y_out_html'>If automatic forwarding does not go.</a>"
				echo '</body></html>'
				#cat $y_out_html
			else
				echo '<html><head>'
				echo '<link rel="stylesheet" type="text/css" href="/Y_Main.css">'
				echo '<link rel="stylesheet" type="text/css" href="/Y_Dist.css">'
				echo '<link rel="stylesheet" type="text/css" href="/Y_User.css">'
				echo '</head>'
				echo '<body>'
				echo "$o"
				echo '</body></html>'
			fi
		else
			msg="$y_install not found"
			y_format_message_html
		fi
	else
		msg="Upload-Problem.<br>Try again, please."
		y_format_message_html
	fi
}

# -----------------------------------------------------------
# extention Installer $1=URL
# -----------------------------------------------------------
do_ext_installer()
{
	if [ -e $y_upload_file ]; then
		rm $y_upload_file
	fi
	wgetlog=`wget -O $y_upload_file $1 2>&1`
	if [ -s "$y_upload_file" ];then
		cd $y_path_tmp
		tar -xf "$y_upload_file"
		rm $y_upload_file
		if [ -s "$y_install" ] #look for install.sh
		then
			chmod 755 $y_install
			o=`$y_install` # execute
			rm -f $y_install # clean up
			echo "ok: wget=$wgetlog"
		fi
	else
		e=`cat /tmp/err.log`
		echo "error: $y_install not found. wget=$wgetlog $e"
	fi
}

do_ext_uninstaller()
{
	uinst="%(CONFIGDIR)/ext/uninstall.sh"
	if [ -e "$uinst"  ]; then
		chmod 755 "$uinst"
		`$uinst $1_uninstall.inc`
	fi 
}

# -----------------------------------------------------------
# view /proc/$1 Informations
# -----------------------------------------------------------
proc()
{
	msg=`cat /proc/$1`
	msg="<b>proc: $1</b><br><br>$msg"
	y_format_message_html
}

# -----------------------------------------------------------
# wake up $1=MAC
# -----------------------------------------------------------
wol()
{
	msg=`ether-wake $1`
	msg="<b>Wake on LAN $1</b><br><br>$msg"
	y_format_message_html
}

# -----------------------------------------------------------
# lcd shot
# $1= optionen | leer
# -----------------------------------------------------------
do_lcshot()
{
	if [ -e "$y_path_varbin/lcshot" ]; then
		$y_path_varbin/lcshot $*
	else
		lcshot $*
	fi
}

# -----------------------------------------------------------
# osd shot
# $1= fbshot | grab
# -----------------------------------------------------------
do_fbshot()
{
	if [ "$1" = "fbshot" ]; then
		shift 1
		if [ -e "$y_path_varbin/fbshot" ]; then
			$y_path_varbin/fbshot $*
		else
			fbshot $*
		fi
	elif [ "$1" = "grab" ]; then
		shift 1
		if [ -e "$y_path_varbin/grab" ]; then
			$y_path_varbin/grab $*
		else
			grab $*
		fi
	fi
}

# -----------------------------------------------------------
# delete fbshot created graphics
# -----------------------------------------------------------
do_fbshot_clear()
{
	rm /tmp/*.bmp
	rm /tmp/*.png
}

# -----------------------------------------------------------
# delete screenshots
# -----------------------------------------------------------
do_screenshot_clear()
{
	rm -f /tmp/*.png
}

# -----------------------------------------------------------
# Settings Backup/Restore
# -----------------------------------------------------------
do_settings_backup_restore()
{
	now=$(date +%Y-%m-%d_%H-%M-%S)
	workdir="$y_path_tmp/y_save_settings/$now"
	case "$1" in
		backup)
			rm -rf $workdir
			mkdir -p $workdir
			backup.sh $workdir >/dev/null
			filename=$(ls -1 -tr $workdir/settings_* | tail -1)
			echo "$filename"
		;;
		restore)
			if [ -s "$y_upload_file" ]
			then
				msg=$(restore.sh "$y_upload_file")
			else
				msg="error: no upload file"
			fi
			y_format_message_html
		;;
	esac
}

restart_neutrino()
{
	echo "fixme"
	#kill -HUP `pidof neutrino`
}

# -----------------------------------------------------------
# Main
# -----------------------------------------------------------
# debug
#echo "call:$*" >> "/tmp/debug.txt"
case "$1" in
	style_set)			style_set $2 ;;
	style_get)			style_get ;;
	image_upload)			image_upload ;;
	image_backup)			image_backup_mtd $2; echo "/tmp/flash_mtd$2.img" ;;
	zapit_upload)			zapit_upload $2 ;;
	kernel-stack)			msg=`dmesg`; y_format_message_html ;;
	ps)				msg=`ps aux`; y_format_message_html ;;
	free)				f=`free`; p=`df -h`; msg="RAM Memory use\n-------------------\n$f\n\nPartitions\n-------------------\n$p"; y_format_message_html ;;
	yreboot)			reboot; echo "Reboot..." ;;
	check_yWeb_conf) 		check_Y_Web_conf ;;
	rcsim)				rcsim $2 >/dev/null ;;
	domount)			shift 1; do_mount $* ;;
	dounmount)			shift 1; do_unmount $* ;;
	cmd)				shift 1; do_cmd $* ;;
	installer)			shift 1; do_installer $* 2>&1 ;;
	ext_uninstaller)		shift 1; do_ext_uninstaller $* 2>&1 ;;
	ext_installer)			shift 1; do_ext_installer $* 2>&1 ;;
	proc)				shift 1; proc $* ;;
	wol)				shift 1; wol $* ;;
	lcshot)				shift 1; do_lcshot $* ;;
	fbshot)				shift 1; do_fbshot $* ;;
	fbshot_clear)			do_fbshot_clear ;;
	screenshot_clear)		do_screenshot_clear ;;
	get_update_version)		wget -O /tmp/version.txt "https://raw.githubusercontent.com/neutrino-images/ni-neutrino/master/data/y-web/Y_Version.txt" ;;
	settings_backup_restore)	shift 1; do_settings_backup_restore $* ;;
	exec_cmd)			shift 1; $* ;;
	automount_list)			shift 1; do_automount_list $* ;;
	automount_getline)		shift 1; do_automount_getline $* ;;
	automount_setline)		shift 1; do_automount_setline $* ;;
	restart_neutrino)		restart_neutrino ;;
	have_plugin_scripts)	 	find %(PLUGINDIR_VAR) -name '*.sh' ;;

	restart_sectionsd)
		killall sectionsd
		sectionsd >/dev/null 2>&1
		msg="sectionsd reboot. ok."
		y_format_message_html
	;;
	get_synctimer_channels)
		if [ -e "$y_path_config/channels.txt" ]
		then
			cat $y_path_config/channels.txt
		else
			cat $y_path_httpd/channels.txt
		fi
	;;
	get_extension_list)
		if [ -e "$y_path_config/extentions.txt" ]
		then
			cat $y_path_config/extentions.txt
		else
			cat $y_path_httpd/extentions.txt
		fi
	;;
	write_extension_list)
		shift 1
		echo  "$*" >$y_path_config/extentions.txt
	;;
	url_get)
		shift 1
		res=`wget -O /tmp/$2 "$1" >/tmp/url.log 2>&1`
		cat /tmp/$2
	;;
	mtd_space|var_space)
		df | while read fs rest; do
			case ${fs:0:3} in
				mtd)
					echo "$fs" "$rest"
					break
				;;
			esac
		done
	;;
	tmp_space)
		df /tmp|grep /tmp
	;;
	get_hostname)
		echo -n $(hostname)
	;;
	*)
		echo "[Y_Tools.sh] Parameter wrong: $*" ;;
esac