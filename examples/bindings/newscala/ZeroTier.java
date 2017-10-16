package zerotier;

public class ZeroTier 
{
	public native int ztjni_socket(int family, int type, int protocol);
    public int socket(int family, int type, int protocol) { return ztjni_socket(family, type, protocol); }
}