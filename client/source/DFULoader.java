package com.controlj.otadfu.backend;

import android.bluetooth.*;
import android.os.Build;
import com.controlj.androidutil.ResourceUtil;
import com.controlj.otadfu.device.BTHandler;

import java.io.IOException;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import static com.controlj.otadfu.backend.BTService.UPLOAD_FILE;
import static com.controlj.otadfu.backend.DFULoader.State.*;

/**
 * Copyright (C) Control-J Pty. Ltd. ACN 103594190
 * All rights reserved
 * <p>
 * User: clyde
 * Date: 14/3/17
 * Time: 14:12
 */
public class DFULoader extends Thread implements BTHandler.Listener, BTHandler.DiscoveryListener {
	public static final String DFU_DATA = "95301002-963F-46B1-B801-0B23E8904835";
	public static final String DFU_CTRL = "95301001-963F-46B1-B801-0B23E8904835";
	public static final String DFU_PROGRESS = "95301003-963F-46B1-B801-0B23E8904835";
	public static final UUID DFU_DATA_UUID = UUID.fromString(DFU_DATA);
	public static final UUID DFU_CTRL_UUID = UUID.fromString(DFU_CTRL);
	public static final UUID DFU_PROG_UUID = UUID.fromString(DFU_PROGRESS);

	static final int DFU_CMD_RESTART = 0x1;     // reset DFU system - resets data counts etc.
	static final int DFU_CMD_DATA = 0x2;     // data coming on the data channel
	static final int DFU_CMD_IV = 0x3;     // iv coming on data channel
	static final int DFU_CMD_DONE = 0x4;     // Data transmission all done.	Response indicates success or not.
	static final int DFU_CMD_RESET = 0x5;     // reset device
	static final int DFU_CMD_DIGEST = 0x6;     // SHA256 digest coming
	static final int DFU_CMD_PING = 0x7;     // check progress

	static final int DFU_CTRL_PKT_CMD = 0;       // offset of command word
	static final int DFU_CTRL_PKT_LEN = 2;       // offset of length word
	static final int DFU_CTRL_PKT_ADR = 4;       // offset of address doubleword
	static final int DFU_CTRL_PKT_SIZE = 8;       // total length of packet

	static final int DFU_RESYNC = 1;            // resync to this address
	static final int DFU_DIGEST_FAILED = 2;		// verification failed

	static final int MAX_RESYNCS = 20;            // max number of times we try to resync

	static final int CHUNK_SIZE = 0x800;        // send in chunks this big - same as FLASH_PAGE_SIZE

	enum State {
		IDLE,
		CONNECTING,
		WRITING,
		ENDING,
		DISCONNECTING
	}

	State state = IDLE;
	private static final int MAX_BUFLEN = 64;
	private static final int MAXQUEUE = 3;
	private static final long TIMEOUT = 10000L;
	private final Semaphore semaphore;
	private final String deviceAddress;
	private final BTService service;
	private final BTHandler btHandler;
	private FirmwareLoader loader;
	private FirmwareLoader.Information info;
	private int resyncVal;
	private int mtu;

	public DFULoader(String deviceAddress, FirmwareLoader loader, BTService service, BTHandler btHandler) throws IOException {
		this.btHandler = btHandler;
		this.semaphore = new Semaphore(0);
		info = loader.getInfo();
		this.deviceAddress = deviceAddress;
		this.service = service;
		this.loader = loader;
	}

	private void acquire(int num) throws InterruptedException {
		if(!semaphore.tryAcquire(num, TIMEOUT, TimeUnit.MILLISECONDS))
			throw new InterruptedException("Timeout");
	}

	private void put4(byte[] packet, long val, int offs) {
		packet[offs] = (byte)(val & 0xFF);
		packet[offs + 1] = (byte)((val >> 8) & 0xFF);
		packet[offs + 2] = (byte)((val >> 16) & 0xFF);
		packet[offs + 3] = (byte)((val >> 24) & 0xFF);
	}

	private int get4(byte[] data, int offset) {
		return (data[offset] & 0xFF) + ((data[offset + 1] & 0xFF) << 8) + ((data[offset + 2] & 0xFF) << 16) + ((data[offset + 3] & 0xFF) << 24);
	}

	private void put2(byte[] packet, int val, int offs) {
		packet[offs] = (byte)(val & 0xFF);
		packet[offs + 1] = (byte)((val >> 8) & 0xFF);
	}

	private void sendCommand(int cmd, int len, long addr) throws InterruptedException {
		byte packet[] = new byte[DFU_CTRL_PKT_SIZE];
		put2(packet, cmd, DFU_CTRL_PKT_CMD);
		put2(packet, len, DFU_CTRL_PKT_LEN);
		put4(packet, addr, DFU_CTRL_PKT_ADR);
		//ResourceUtil.logMsg("Sendcommand - permits available %d", semaphore.availablePermits());
		acquire(1);
		//ResourceUtil.logMsg("Sendcommand - writing command %d, len %d", cmd, addr);
		btHandler.writeRequest(deviceAddress, DFU_CTRL_UUID, packet);
	}

	private void sendCommand(int cmd) throws InterruptedException {
		sendCommand(cmd, 0, 0);
	}

	@Override
	public void run() {
		state = CONNECTING;
		int progress = 0;
		int resyncs = 0;
		service.sendResult(BTService.UPLOAD_PROGRESS, 0, deviceAddress);
		btHandler.connectRequest(deviceAddress, true, this, MAX_BUFLEN + 4 + 5);
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
			btHandler.priorityRequest(deviceAddress, BluetoothGatt.CONNECTION_PRIORITY_HIGH);
		btHandler.discoveryRequest(deviceAddress, this);
		int totalBytes = info.getTotalBytes();
		int totalCount = 0;
		btHandler.notificationRequest(deviceAddress, DFU_PROG_UUID, true);
		try {
			sendCommand(DFU_CMD_RESTART);
			if(mtu < MAX_BUFLEN + 4 + 3) {
				ResourceUtil.logMsg("MTU of %d is insufficient");
				service.sendResult(BTService.OOPS, BTService.UPLOAD_FILE, "invalid mtu");
			}
			for(int i = 0; i != info.getNumBlocks(); i++) {
				FirmwareLoader.DataHeader header = loader.getHeader(i);
				header.start();
				ResourceUtil.logMsg("Writing %d bytes at %X", header.getLength(), header.getAddr());
				int length = header.getLength() + header.getExtra();
				byte[] iv = header.getInitVector();
				sendCommand(DFU_CMD_IV, iv.length, 0);
				acquire(1);
				btHandler.writeRequest(deviceAddress, DFU_DATA_UUID, iv, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
				int addr = header.getAddr();
				sendCommand(DFU_CMD_DATA, length, addr);
				int count = 0;
				resyncVal = addr + length;
				int chunks = addr / CHUNK_SIZE;
				while(count != length) {
					int balance = Math.min(length - count, MAX_BUFLEN);
					byte[] buffer = new byte[balance + 4];
					put4(buffer, addr, 0);
					header.seek(count);
					header.read(buffer, 4);
					acquire(1);
					btHandler.writeRequest(deviceAddress, DFU_DATA_UUID, buffer, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
					count += balance;
					addr += balance;
					totalCount += balance;
					// wait for device to catch up after each chunk.
					if(count == length || addr / CHUNK_SIZE != chunks) {
						chunks = addr / CHUNK_SIZE;
						sendCommand(DFU_CMD_PING, count, 0);
					}
					if(totalCount * 100 / totalBytes != progress) {
						progress = totalCount * 100 / totalBytes;
						service.sendResult(BTService.UPLOAD_PROGRESS, progress);
					}
					synchronized(this) {
						if(resyncVal < addr) {
							if(++resyncs > MAX_RESYNCS)
								interrupt();
							resyncVal &= ~(MAX_BUFLEN - 1);
							totalCount -= addr - resyncVal;
							addr = resyncVal;
							resyncVal = header.getAddr()+length;
							count = addr - header.getAddr();
						}
					}
				}
				byte[] digest = header.getDigest();
				sendCommand(DFU_CMD_DIGEST, header.getLength() + header.getExtra(), header.getAddr());
				acquire(1);
				btHandler.writeRequest(deviceAddress, DFU_DATA_UUID, digest, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
				ResourceUtil.logMsg("Done...");
			}
			state = ENDING;
			sendCommand(DFU_CMD_DONE);
			sendCommand(DFU_CMD_RESET);
			acquire(MAXQUEUE);
			state = DISCONNECTING;
			service.sendResult(UPLOAD_FILE, totalBytes);
			// don't bother waiting
		} catch(InterruptedException e) {
			service.sendResult(BTService.OOPS, UPLOAD_FILE, e.getMessage());
			btHandler.connectRequest(deviceAddress, false, null);
		} catch(IOException e) {
			ResourceUtil.logMsg("Exception reading file %s: %s", loader.getInfo().getFilename(), e.toString());
			service.sendResult(BTService.OOPS, UPLOAD_FILE, e.toString());
		} finally {
			try {
				loader.close();
			} catch(IOException ignored) {
			}
			ResourceUtil.logMsg("Disconnecting...");
			btHandler.connectRequest(deviceAddress, false, null);
		}
	}

	@Override
	public void onConnected(BluetoothDevice device) {
	}

	@Override
	public void onServicesDiscovered(BluetoothDevice device, List<BluetoothGattService> services) {
		ResourceUtil.logMsg("Device discovered");
		state = WRITING;
		semaphore.release(MAXQUEUE);
	}

	@Override
	public void onDisconnected(BluetoothDevice device) {
		if(state != DISCONNECTING) {
			ResourceUtil.logMsg("Bluetooth unexpected disconnect %s", device.getAddress());
			interrupt();
		}
	}

	@Override
	public void onCharacteristicRead(BluetoothDevice device, BluetoothGattCharacteristic characteristic) {
		byte[] val = characteristic.getValue();
		//ResourceUtil.logMsg("Characteristic %s read, length %d", characteristic.getUuid().toString(), characteristic.getValue().length);
		if(DFU_PROGRESS.equalsIgnoreCase(characteristic.getUuid().toString())) {
			if(val.length >= 1) {
				switch(val[0]) {
					case DFU_RESYNC:
						synchronized(this) {
							resyncVal = get4(val, 1);
						}
						ResourceUtil.logMsg("Resync to %x", get4(val, 1));
						break;

					case DFU_DIGEST_FAILED:
						ResourceUtil.logMsg("Digest failed");
						service.sendResult(BTService.OOPS, BTService.UPLOAD_FILE, "verification failed");
						interrupt();
						break;

					default:
						ResourceUtil.logMsg("Received unknown command on progress characteristic: %d", val[0]);
						break;
				}
			}
		}
	}

	@Override
	public void onDescriptorRead(BluetoothDevice device, BluetoothGattDescriptor descriptor) {

	}

	@Override
	public void onError(BluetoothDevice device, int code) {
		ResourceUtil.logMsg("Bluetooth error on %s: %d", device.getAddress(), code);
		interrupt();
	}

	@Override
	public void onCharacteristicWritten(BluetoothDevice device, BluetoothGattCharacteristic characteristic) {
		//ResourceUtil.logMsg("On characteristic written: %s", characteristic.getUuid().toString());
		semaphore.release();
	}

	@Override
	public void onMtuChanged(BluetoothDevice device, int mtu) {
		ResourceUtil.logMsg("MTU changed to %d", mtu);
		this.mtu = mtu;
	}
}

