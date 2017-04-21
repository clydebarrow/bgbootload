package com.controlj.otadfu.backend;

import android.os.Parcel;
import android.os.Parcelable;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Locale;
import java.util.UUID;

/**
 * Copyright (C) Control-J Pty. Ltd. ACN 103594190
 * All rights reserved
 * <p>
 * User: clyde
 * Date: 26/3/17
 * Time: 12:07
 */

/*
This is the structure of the firmware file header, and block headers.
The file consists of the file header, followed by numblocks block headers, then the
data at the specified offsets.
typedef struct {
    unsigned char tag[4];            // magic number goes here
    unsigned char major[2];        // major version number
    unsigned char minor[2];        // minor version number
    unsigned char numblocks[2];        // the number of blocks in the file,
    unsigned char unused[6];
    unsigned char service_uuid[UUID_LEN];    // the service uuid of the bootloader
} firmware;

//Block header

typedef struct {
    unsigned char addr[4];        // the address at which to load this block
    unsigned char size[4];        // the size of this block
    unsigned char offset[4];        // offset in the file of the data
    unsigned char padding[1];   // length of padding at end
    unsigned char unused[3];
    unsigned char init_vector[IV_LEN];    // the block 0 initialization vector
	unsigned char sha256[32];       // SHA256 hash of the data
} block_header;


 */
public class FirmwareLoader {
	static final int UUID_LEN = 16;        // length of uuid
	static final int MAGIC = 0x55A322BF;
	static final int MAGIC_OFFS = 0;
	static final int MAJOR_OFFS = 4;
	static final int MINOR_OFFS = 6;
	static final int NUMBLK_OFFS = 8;
	static final int UUID_OFFS = 16;
	static final int HEADER_LEN = (16 + UUID_LEN);

	static final int DIGEST_LEN = 32;		// length of SHA256 digest
	static final int IV_LEN = (128 / 8);        // length of initialization vector
	static final int ADDR_OFFS = 0;
	static final int LENGTH_OFFS = 4;
	static final int OFFSET_OFFS = 8;
	static final int EXTRA_OFFS = 12;
	static final int IV_OFFS = 16;
	static final int DIGEST_OFFS = IV_OFFS + IV_LEN;
	static final int BLKHDR_LEN = (DIGEST_OFFS + DIGEST_LEN);

	private FileInputStream inputStream;
	private RandomAccessFile randomAccessFile;
	private Information info;
	private ArrayList<DataHeader> headers = new ArrayList<>();

	class DataHeader {
		private int offset;        // file offset
		private int addr;        // address in memory
		private int length;        // number of bytes
		private int extra;        // extra bytes at end
		private byte[] initVector = new byte[IV_LEN];
		private byte[] digest = new byte[DIGEST_LEN];

		public int getAddr() {
			return addr;
		}

		public int getLength() {
			return length;
		}

		public int getExtra() {
			return extra;
		}

		public void start() throws IOException {
			if(randomAccessFile == null)
				randomAccessFile = new RandomAccessFile(info.filename, "r");
			randomAccessFile.seek(offset);
		}

		public void seek(int position) throws IOException {
			randomAccessFile.seek(offset+position);
		}

		public int read(byte[] buf, int bufferOffs) throws IOException {
			int len = buf.length-bufferOffs;
			long pos = randomAccessFile.getFilePointer();
			if(pos + len > offset + length + extra)
				len = (int)(offset + length + extra - pos);
			if(len == 0)
				return 0;
			return randomAccessFile.read(buf, bufferOffs, len);
		}

		public byte[] getInitVector() {
			return initVector;
		}

		public byte[] getDigest() {
			return digest;
		}
	}

	public static class Information implements Parcelable {
		private String filename;
		private UUID serviceUuid;
		private int totalBytes;
		private int versionMajor, versionMinor, numBlocks, baseAddr;

		private Information() {

		}

		protected Information(Parcel in) {
			filename = in.readString();
			totalBytes = in.readInt();
			versionMajor = in.readInt();
			versionMinor = in.readInt();
			numBlocks = in.readInt();
			baseAddr = in.readInt();
		}

		@Override
		public void writeToParcel(Parcel dest, int flags) {
			dest.writeString(filename);
			dest.writeInt(totalBytes);
			dest.writeInt(versionMajor);
			dest.writeInt(versionMinor);
			dest.writeInt(numBlocks);
			dest.writeInt(baseAddr);
		}

		@Override
		public int describeContents() {
			return 0;
		}

		public static final Creator<Information> CREATOR = new Creator<Information>() {
			@Override
			public Information createFromParcel(Parcel in) {
				return new Information(in);
			}

			@Override
			public Information[] newArray(int size) {
				return new Information[size];
			}
		};

		public String getFilename() {
			return filename;
		}

		public UUID getServiceUuid() {
			return serviceUuid;
		}

		public int getVersionMajor() {
			return versionMajor;
		}

		public int getVersionMinor() {
			return versionMinor;
		}

		public int getNumBlocks() {
			return numBlocks;
		}

		public int getBaseAddr() {
			return baseAddr;
		}

		Information(String filename, byte[] initVector, UUID serviceUuid, int versionMajor, int versionMinor, int numBlocks, int baseAddr) {
			this.filename = filename;
			this.serviceUuid = serviceUuid;
			this.versionMajor = versionMajor;
			this.versionMinor = versionMinor;
			this.numBlocks = numBlocks;
			this.baseAddr = baseAddr;
		}

		@Override
		public String toString() {
			return String.format(Locale.US, "%s: V%d.%d, %d blocks, base %X, uuid: %s",
					filename, versionMajor, versionMinor, numBlocks, baseAddr, serviceUuid.toString());
		}

		public int getTotalBytes() {
			return totalBytes;
		}
	}

	public Information getInfo() {
		return info;
	}

	public FirmwareLoader(String filename) throws IOException {
		info = new Information();
		info.filename = filename;
		File f = new File(filename);
		inputStream = new FileInputStream(f);
		inputStream.mark((int)f.length());
		byte[] buffer = new byte[HEADER_LEN];
		if(inputStream.read(buffer) != buffer.length)
			throw new IOException("Short read on file header");
		ByteBuffer bb = ByteBuffer.wrap(buffer);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		if(bb.getInt(MAGIC_OFFS) != MAGIC)
			throw new IOException(String.format(Locale.US, "Invalid magic number - expected %X, saw %X", MAGIC, bb.getInt(MAGIC_OFFS)));
		info.versionMajor = bb.getChar(MAJOR_OFFS);
		info.versionMinor = bb.getChar(MINOR_OFFS);
		info.numBlocks = bb.getChar(NUMBLK_OFFS);
		if(info.numBlocks == 0)
			throw new IOException("Block count is zero");
		// the uuid is in big-endian format
		bb.order(ByteOrder.BIG_ENDIAN);
		long low = bb.getLong(UUID_OFFS + 8);
		long high = bb.getLong(UUID_OFFS);
		info.serviceUuid = new UUID(high, low);
		info.baseAddr = Integer.MAX_VALUE;
		for(int i = 0; i != info.numBlocks; i++)
			readHeader();
		inputStream.close();
	}

	private void readHeader() throws IOException {
		byte[] buf = new byte[BLKHDR_LEN];
		if(inputStream.read(buf) != buf.length)
			throw new IOException("Short read on block header");
		DataHeader hdr = new DataHeader();
		ByteBuffer bb = ByteBuffer.wrap(buf);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		hdr.length = bb.getInt(LENGTH_OFFS);
		hdr.addr = bb.getInt(ADDR_OFFS);
		hdr.offset = bb.getInt(OFFSET_OFFS);
		hdr.extra = bb.get(EXTRA_OFFS);
		bb.position(IV_OFFS);
		bb.get(hdr.initVector);
		bb.position(DIGEST_OFFS);
		bb.get(hdr.digest);
		info.totalBytes += hdr.length + hdr.extra;
		headers.add(hdr);
		if(hdr.addr < info.baseAddr)
			info.baseAddr = hdr.addr;
	}

	public void close() throws IOException {
		if(randomAccessFile != null)
			randomAccessFile.close();
	}

	public DataHeader getHeader(int idx) {
		return headers.get(idx);
	}
}
