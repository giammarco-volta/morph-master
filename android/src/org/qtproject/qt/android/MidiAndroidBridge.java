package org.qtproject.qt.android;

import android.content.Context;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;

public class MidiAndroidBridge {
    private static final HashMap<Integer, MidiDevice> openDevices = new HashMap<>();

    private static MidiOutputPort currentInputPort = null;
    private static MidiInputPort currentOutputPort = null;

    public static void keepScreenOn() {
        android.app.Activity activity = QtNative.activity();
        if (activity == null)
            return;

        activity.runOnUiThread(() -> {
            activity.getWindow().addFlags(
                android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            android.util.Log.d("MORPHMASTER_MIDI", "KEEP_SCREEN_ON enabled");
        });
    }

    private interface DeviceReadyCallback {
        void onReady(MidiDevice device);
    }

    private static native void nativeOnMidiReceived(byte[] data, int count, long timestamp);

    private static Context context() {
        return QtNative.activity();
    }

    public static String[] listInputs() {
        Log.d("MORPHMASTER_MIDI", "listInputs called");

        MidiManager manager = midiManager();
        if (manager == null)
            return new String[0];

        MidiDeviceInfo[] devices = manager.getDevices();
        Log.d("MORPHMASTER_MIDI", "devices count = " + devices.length);

        ArrayList<String> result = new ArrayList<>();

        for (MidiDeviceInfo device : devices) {
            logDevice(device);

            for (int port = 0; port < device.getOutputPortCount(); ++port) {
                result.add(deviceName(device) + " / Output " + port);
            }
        }

        return result.toArray(new String[0]);
    }

    public static String[] listOutputs() {
        Log.d("MORPHMASTER_MIDI", "listOutputs called");

        MidiManager manager = midiManager();
        if (manager == null)
            return new String[0];

        MidiDeviceInfo[] devices = manager.getDevices();
        Log.d("MORPHMASTER_MIDI", "devices count = " + devices.length);

        ArrayList<String> result = new ArrayList<>();

        for (MidiDeviceInfo device : devices) {
            logDevice(device);

            for (int port = 0; port < device.getInputPortCount(); ++port) {
                result.add(deviceName(device) + " / Input " + port);
            }
        }

        return result.toArray(new String[0]);
    }

    public static boolean openInput(int index) {
        Log.d("MORPHMASTER_MIDI", "openInput called index=" + index);

        closeInput();

        MidiManager manager = midiManager();
        if (manager == null)
            return false;

        ArrayList<MidiDeviceInfo> devicesList = new ArrayList<>();
        ArrayList<Integer> portList = new ArrayList<>();

        for (MidiDeviceInfo device : manager.getDevices()) {
            for (int port = 0; port < device.getOutputPortCount(); ++port) {
                devicesList.add(device);
                portList.add(port);
            }
        }

        if (index < 0 || index >= devicesList.size()) {
            Log.d("MORPHMASTER_MIDI", "openInput: invalid index");
            return false;
        }

        MidiDeviceInfo deviceInfo = devicesList.get(index);
        int portNumber = portList.get(index);

        openOrReuseDevice(manager, deviceInfo, device -> {
            currentInputPort = device.openOutputPort(portNumber);

            if (currentInputPort == null) {
                Log.d("MORPHMASTER_MIDI", "openInput: openOutputPort returned null");
                return;
            }

            currentInputPort.connect(new MidiReceiver() {
                @Override
                public void onSend(byte[] data, int offset, int count, long timestamp) {
                    byte[] msg = new byte[count];
                    System.arraycopy(data, offset, msg, 0, count);
                    nativeOnMidiReceived(msg, count, timestamp);
                }
            });

            Log.d("MORPHMASTER_MIDI",
                    "input opened: " + deviceName(deviceInfo) + " port " + portNumber);
        });

        return true;
    }

    public static boolean openOutput(int index) {
        Log.d("MORPHMASTER_MIDI", "openOutput called index=" + index);

        closeOutput();

        MidiManager manager = midiManager();
        if (manager == null)
            return false;

        ArrayList<MidiDeviceInfo> devicesList = new ArrayList<>();
        ArrayList<Integer> portList = new ArrayList<>();

        for (MidiDeviceInfo device : manager.getDevices()) {
            for (int port = 0; port < device.getInputPortCount(); ++port) {
                devicesList.add(device);
                portList.add(port);
            }
        }

        if (index < 0 || index >= devicesList.size()) {
            Log.d("MORPHMASTER_MIDI", "openOutput: invalid index");
            return false;
        }

        MidiDeviceInfo deviceInfo = devicesList.get(index);
        int portNumber = portList.get(index);

        openOrReuseDevice(manager, deviceInfo, device -> {
            currentOutputPort = device.openInputPort(portNumber);

            if (currentOutputPort == null) {
                Log.d("MORPHMASTER_MIDI", "openOutput: openInputPort returned null");
                return;
            }

            Log.d("MORPHMASTER_MIDI",
                    "output opened: " + deviceName(deviceInfo) + " port " + portNumber);
        });

        return true;
    }

    public static void closeInput() {
        try {
            if (currentInputPort != null) {
                currentInputPort.close();
                currentInputPort = null;
                Log.d("MORPHMASTER_MIDI", "input port closed");
            }
        } catch (Exception e) {
            Log.d("MORPHMASTER_MIDI", "closeInput exception: " + e);
        }
    }

    public static void closeOutput() {
        try {
            if (currentOutputPort != null) {
                currentOutputPort.close();
                currentOutputPort = null;
                Log.d("MORPHMASTER_MIDI", "output port closed");
            }
        } catch (Exception e) {
            Log.d("MORPHMASTER_MIDI", "closeOutput exception: " + e);
        }
    }

    public static boolean sendShort(int status, int data1, int data2) {
        if (currentOutputPort == null) {
            Log.d("MORPHMASTER_MIDI", "sendShort: output port is null");
            return false;
        }

        int messageType = status & 0xF0;

        int length;
        switch (messageType) {
            case 0xC0: // Program Change
            case 0xD0: // Channel Pressure
                length = 2;
                break;

            default:
                length = 3;
                break;
        }

        byte[] msg = new byte[length];

        msg[0] = (byte)(status & 0xFF);
        msg[1] = (byte)(data1 & 0x7F);

        if (length == 3)
            msg[2] = (byte)(data2 & 0x7F);

        try {
            currentOutputPort.send(msg, 0, length);
            return true;
        } catch (Exception e) {
            Log.d("MORPHMASTER_MIDI", "sendShort exception: " + e);
            return false;
        }
    }

    public static boolean sendBytes(byte[] data) {
        if (currentOutputPort == null || data == null) {
            Log.d("MORPHMASTER_MIDI", "sendBytes: output port or data is null");
            return false;
        }

        try {
            currentOutputPort.send(data, 0, data.length);
            return true;
        } catch (Exception e) {
            Log.d("MORPHMASTER_MIDI", "sendBytes exception: " + e);
            return false;
        }
    }

    private static MidiManager midiManager() {
        Context ctx = context();
        if (ctx == null) {
            Log.d("MORPHMASTER_MIDI", "context is null");
            return null;
        }

        MidiManager manager = (MidiManager) ctx.getSystemService(Context.MIDI_SERVICE);
        if (manager == null) {
            Log.d("MORPHMASTER_MIDI", "MidiManager is null");
            return null;
        }

        return manager;
    }

    private static void openOrReuseDevice(
            MidiManager manager,
            MidiDeviceInfo deviceInfo,
            DeviceReadyCallback callback) {
        int id = deviceInfo.getId();

        MidiDevice existing = openDevices.get(id);
        if (existing != null) {
            Log.d("MORPHMASTER_MIDI", "reusing device id=" + id);
            callback.onReady(existing);
            return;
        }

        manager.openDevice(deviceInfo, device -> {
            if (device == null) {
                Log.d("MORPHMASTER_MIDI", "openDevice returned null for id=" + id);
                return;
            }

            openDevices.put(id, device);
            Log.d("MORPHMASTER_MIDI", "device opened id=" + id);
            callback.onReady(device);
        }, new Handler(Looper.getMainLooper()));
    }

    private static void logDevice(MidiDeviceInfo device) {
        Log.d("MORPHMASTER_MIDI",
                "device " + device.getId()
                        + " name=" + deviceName(device)
                        + " inputs=" + device.getInputPortCount()
                        + " outputs=" + device.getOutputPortCount());
    }

    private static String deviceName(MidiDeviceInfo device) {
        String name = device.getProperties().getString(MidiDeviceInfo.PROPERTY_NAME);
        if (name != null && !name.isEmpty())
            return name;

        String manufacturer = device.getProperties().getString(MidiDeviceInfo.PROPERTY_MANUFACTURER);
        String product = device.getProperties().getString(MidiDeviceInfo.PROPERTY_PRODUCT);

        if (manufacturer != null && product != null)
            return manufacturer + " " + product;

        return "MIDI Device " + device.getId();
    }
}