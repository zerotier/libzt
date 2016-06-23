package com.zerotier.one.service;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import com.zerotier.sdk.DataStoreGetListener;
import com.zerotier.sdk.DataStorePutListener;

public class DataStore implements DataStoreGetListener, DataStorePutListener {
    private static final String TAG = "DataStore";
    private Context _ctx;

    public DataStore(Context ctx) {
        this._ctx = ctx;
    }

    @Override
    public int onDataStorePut(String name, byte[] buffer, boolean secure) {
        Log.d(TAG, "Writing File: " + name);
        try {
            if(name.contains("/")) {
                // filename has a directory separator.
                int ix = name.lastIndexOf('/');
                String path = name.substring(0, ix);

                File f = new File(_ctx.getFilesDir(), path);
                if(!f.exists()) {
                    f.mkdirs();
                }

                File outputFile = new File(f, name.substring(name.lastIndexOf("/")+1));
                FileOutputStream fos = new FileOutputStream(outputFile);
                fos.write(buffer);
                fos.flush();
                fos.close();
                return 0;
            } else {

                FileOutputStream fos = _ctx.openFileOutput(name, Context.MODE_PRIVATE);
                fos.write(buffer);
                fos.flush();
                fos.close();
                return 0;
            }
        } catch (FileNotFoundException fnf) {
            fnf.printStackTrace();
            return -1;
        } catch (IOException io) {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            io.printStackTrace(pw);
            Log.e(TAG, sw.toString());
            return -2;
        } catch(IllegalArgumentException ie) {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            ie.printStackTrace(pw);
            Log.e(TAG, sw.toString());
            return -3;
        }
    }

    @Override
    public int onDelete(String name) {
        Log.d(TAG, "Deleting File: " + name);
        boolean deleted = false;
        if(name.contains("/")) {
            // filename has a directory separator.

            File f = new File(_ctx.getFilesDir(), name);
            if (!f.exists()) {
                deleted = true;
            } else {
                deleted = f.delete();
            }
        } else {
            deleted = _ctx.deleteFile(name);
        }
        return deleted ? 0 : 1;
    }

    @Override
    public long onDataStoreGet(String name, byte[] out_buffer,
                               long bufferIndex, long[] out_objectSize) {
        Log.d(TAG, "Reading File: " + name);
        try {
            if(name.contains("/")) {
                // filename has a directory separator.
                int ix = name.lastIndexOf('/');
                String path = name.substring(0, ix);

                File f = new File(_ctx.getFilesDir(), path);
                if(!f.exists()) {
                    f.mkdirs();
                }

                File inputFile = new File(f, name.substring(name.lastIndexOf('/')+1));
                if(!inputFile.exists()) {
                    return 0;
                }

                FileInputStream fin = new FileInputStream(inputFile);
                if(bufferIndex > 0) {
                    fin.skip(bufferIndex);
                }
                int read = fin.read(out_buffer);
                fin.close();
                return read;
            } else {
                FileInputStream fin = _ctx.openFileInput(name);
                out_objectSize[0] = fin.getChannel().size();
                if (bufferIndex > 0) {
                    fin.skip(bufferIndex);
                }
                int read = fin.read(out_buffer);
                fin.close();
                return read;
            }
        } catch (FileNotFoundException fnf) {
            // Can't read a file that doesn't exist!
            out_objectSize[0] = 0;
            return -1;
        } catch (IOException io) {
            io.printStackTrace();
            return -2;
        } catch (Exception ex) {
            ex.printStackTrace();
            return -3;
        }
    }


}
