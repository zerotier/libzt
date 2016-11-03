# Builds a test docker image

test_name=${PWD##*/}
echo 'Building dockerfiles for test: ' "$test_name"
touch "$test_name".name

# Docker won't allow the inclusion of files outside of the build directory
cp ../../*.conf .
cp ../../zerotier-one zerotier-one
cp ../../zerotier-cli zerotier-cli
cp ../../zerotier-cli zerotier-sdk-service
cp ../../zerotier-intercept zerotier-intercept
cp ../../libztintercept.so libztintercept.so
cp ../../liblwip.so liblwip.so
cp ../../sdk_identity.public sdk_identity.public
cp ../../sdk_identity.secret sdk_identity.secret
cp ../../monitor_identity.public monitor_identity.public
cp ../../monitor_identity.secret monitor_identity.secret

docker build --tag="$test_name" -f sdk_dockerfile .
docker build --tag="$test_name"_monitor -f monitor_dockerfile .

rm -f zerotier-cli
rm -f zerotier-sdk-service
rm -f zerotier-intercept
rm -f *.so
rm -f *.public
rm -f *.secret
rm -f *.conf
rm -f *.name