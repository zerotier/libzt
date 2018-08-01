# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.http_obj.Debug:
PostBuild.http_pic.Debug:
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libhttp_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libhttp_pic.a


PostBuild.lwip_obj.Debug:
PostBuild.lwip_pic.Debug:
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/liblwip_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Debug/liblwip_pic.a


PostBuild.zt-shared.Debug:
PostBuild.lwip_pic.Debug: /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-shared.dylib
PostBuild.zto_pic.Debug: /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-shared.dylib
PostBuild.http_pic.Debug: /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-shared.dylib
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-shared.dylib:\
	/Users/joseph/op/zt/libzt_new/bin/lib/Debug/liblwip_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzto_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libhttp_pic.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-shared.dylib


PostBuild.zt-static.Debug:
PostBuild.lwip_obj.Debug: /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-static.a
PostBuild.zto_obj.Debug: /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-static.a
PostBuild.http_obj.Debug: /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-static.a
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-static.a:\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Debug/lwip_obj.build/Objects-normal/liblwip_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Debug/zto_obj.build/Objects-normal/libzto_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Debug/http_obj.build/Objects-normal/libhttp_obj.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzt-static.a


PostBuild.zto_obj.Debug:
PostBuild.zto_pic.Debug:
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzto_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzto_pic.a


PostBuild.http_obj.Release:
PostBuild.http_pic.Release:
/Users/joseph/op/zt/libzt_new/bin/lib/Release/libhttp_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Release/libhttp_pic.a


PostBuild.lwip_obj.Release:
PostBuild.lwip_pic.Release:
/Users/joseph/op/zt/libzt_new/bin/lib/Release/liblwip_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Release/liblwip_pic.a


PostBuild.zt-shared.Release:
PostBuild.lwip_pic.Release: /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-shared.dylib
PostBuild.zto_pic.Release: /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-shared.dylib
PostBuild.http_pic.Release: /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-shared.dylib
/Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-shared.dylib:\
	/Users/joseph/op/zt/libzt_new/bin/lib/Release/liblwip_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/Release/libzto_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/Release/libhttp_pic.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-shared.dylib


PostBuild.zt-static.Release:
PostBuild.lwip_obj.Release: /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-static.a
PostBuild.zto_obj.Release: /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-static.a
PostBuild.http_obj.Release: /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-static.a
/Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-static.a:\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Release/lwip_obj.build/Objects-normal/liblwip_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Release/zto_obj.build/Objects-normal/libzto_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Release/http_obj.build/Objects-normal/libhttp_obj.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzt-static.a


PostBuild.zto_obj.Release:
PostBuild.zto_pic.Release:
/Users/joseph/op/zt/libzt_new/bin/lib/Release/libzto_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/Release/libzto_pic.a


PostBuild.http_obj.MinSizeRel:
PostBuild.http_pic.MinSizeRel:
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libhttp_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libhttp_pic.a


PostBuild.lwip_obj.MinSizeRel:
PostBuild.lwip_pic.MinSizeRel:
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/liblwip_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/liblwip_pic.a


PostBuild.zt-shared.MinSizeRel:
PostBuild.lwip_pic.MinSizeRel: /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-shared.dylib
PostBuild.zto_pic.MinSizeRel: /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-shared.dylib
PostBuild.http_pic.MinSizeRel: /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-shared.dylib
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-shared.dylib:\
	/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/liblwip_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzto_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libhttp_pic.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-shared.dylib


PostBuild.zt-static.MinSizeRel:
PostBuild.lwip_obj.MinSizeRel: /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-static.a
PostBuild.zto_obj.MinSizeRel: /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-static.a
PostBuild.http_obj.MinSizeRel: /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-static.a
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-static.a:\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/MinSizeRel/lwip_obj.build/Objects-normal/liblwip_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/MinSizeRel/zto_obj.build/Objects-normal/libzto_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/MinSizeRel/http_obj.build/Objects-normal/libhttp_obj.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzt-static.a


PostBuild.zto_obj.MinSizeRel:
PostBuild.zto_pic.MinSizeRel:
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzto_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzto_pic.a


PostBuild.http_obj.RelWithDebInfo:
PostBuild.http_pic.RelWithDebInfo:
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libhttp_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libhttp_pic.a


PostBuild.lwip_obj.RelWithDebInfo:
PostBuild.lwip_pic.RelWithDebInfo:
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/liblwip_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/liblwip_pic.a


PostBuild.zt-shared.RelWithDebInfo:
PostBuild.lwip_pic.RelWithDebInfo: /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-shared.dylib
PostBuild.zto_pic.RelWithDebInfo: /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-shared.dylib
PostBuild.http_pic.RelWithDebInfo: /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-shared.dylib
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-shared.dylib:\
	/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/liblwip_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzto_pic.a\
	/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libhttp_pic.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-shared.dylib


PostBuild.zt-static.RelWithDebInfo:
PostBuild.lwip_obj.RelWithDebInfo: /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-static.a
PostBuild.zto_obj.RelWithDebInfo: /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-static.a
PostBuild.http_obj.RelWithDebInfo: /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-static.a
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-static.a:\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/RelWithDebInfo/lwip_obj.build/Objects-normal/liblwip_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/RelWithDebInfo/zto_obj.build/Objects-normal/libzto_obj.a\
	/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/RelWithDebInfo/http_obj.build/Objects-normal/libhttp_obj.a
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzt-static.a


PostBuild.zto_obj.RelWithDebInfo:
PostBuild.zto_pic.RelWithDebInfo:
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzto_pic.a:
	/bin/rm -f /Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzto_pic.a




# For each target create a dummy ruleso the target does not have to exist
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libhttp_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/liblwip_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/Debug/libzto_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libhttp_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/liblwip_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/MinSizeRel/libzto_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libhttp_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/liblwip_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/RelWithDebInfo/libzto_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/Release/libhttp_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/Release/liblwip_pic.a:
/Users/joseph/op/zt/libzt_new/bin/lib/Release/libzto_pic.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Debug/http_obj.build/Objects-normal/libhttp_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Debug/lwip_obj.build/Objects-normal/liblwip_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Debug/zto_obj.build/Objects-normal/libzto_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/MinSizeRel/http_obj.build/Objects-normal/libhttp_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/MinSizeRel/lwip_obj.build/Objects-normal/liblwip_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/MinSizeRel/zto_obj.build/Objects-normal/libzto_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/RelWithDebInfo/http_obj.build/Objects-normal/libhttp_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/RelWithDebInfo/lwip_obj.build/Objects-normal/liblwip_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/RelWithDebInfo/zto_obj.build/Objects-normal/libzto_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Release/http_obj.build/Objects-normal/libhttp_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Release/lwip_obj.build/Objects-normal/liblwip_obj.a:
/Users/joseph/op/zt/libzt_new/packages/iOS/libzt.build/Release/zto_obj.build/Objects-normal/libzto_obj.a:
