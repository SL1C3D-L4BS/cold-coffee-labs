//! Content-Length framing for JSON-RPC over stdio (LSP-style).
//!
//! Each message is preceded by one or more HTTP-like headers terminated
//! by a blank line, followed by exactly `Content-Length` bytes of payload.
//!
//! ```text
//! Content-Length: 123\r\n
//! \r\n
//! {"jsonrpc":"2.0",...}
//! ```
//!
//! This module is sync and `std`-only — it operates on buffers, so it
//! can be driven by either the sync stdio in tests or the async
//! `runtime` module behind the `runtime` feature.

use std::io::{self, BufRead, Write};

/// Framing errors.
#[derive(Debug, thiserror::Error)]
pub enum FrameError {
    /// Upstream I/O failure.
    #[error("io: {0}")]
    Io(#[from] io::Error),
    /// Header line was not valid UTF-8.
    #[error("bad header: {0}")]
    BadHeader(String),
    /// Required `Content-Length` header was missing.
    #[error("missing Content-Length header")]
    MissingContentLength,
    /// The provided content-length was malformed.
    #[error("invalid Content-Length: {0}")]
    InvalidContentLength(String),
    /// Upstream closed (EOF) before a complete frame arrived.
    #[error("connection closed mid-frame")]
    UnexpectedEof,
}

/// Write a single framed JSON-RPC message.
pub fn write_frame(w: &mut impl Write, body: &[u8]) -> Result<(), FrameError> {
    write!(w, "Content-Length: {}\r\n\r\n", body.len())?;
    w.write_all(body)?;
    w.flush()?;
    Ok(())
}

/// Read a single framed message from a `BufRead`. Returns `Ok(None)` on EOF
/// between frames (clean shutdown).
pub fn read_frame<R: BufRead>(r: &mut R) -> Result<Option<Vec<u8>>, FrameError> {
    let mut content_length: Option<usize> = None;
    let mut header_count = 0usize;
    loop {
        let mut line = String::new();
        let n = r.read_line(&mut line)?;
        if n == 0 {
            // Clean EOF if we haven't started a frame.
            if header_count == 0 {
                return Ok(None);
            }
            return Err(FrameError::UnexpectedEof);
        }
        header_count += 1;
        // Blank line terminates headers.
        if line == "\r\n" || line == "\n" {
            break;
        }
        let (key, value) = line
            .split_once(':')
            .ok_or_else(|| FrameError::BadHeader(line.trim().to_owned()))?;
        if key.trim().eq_ignore_ascii_case("content-length") {
            let v = value.trim();
            let cl: usize = v
                .parse()
                .map_err(|_| FrameError::InvalidContentLength(v.to_owned()))?;
            content_length = Some(cl);
        }
        // Unknown headers are silently ignored per LSP/MCP spec.
    }
    let cl = content_length.ok_or(FrameError::MissingContentLength)?;
    let mut buf = vec![0u8; cl];
    read_exact(r, &mut buf)?;
    Ok(Some(buf))
}

fn read_exact<R: BufRead>(r: &mut R, buf: &mut [u8]) -> Result<(), FrameError> {
    let mut read = 0;
    while read < buf.len() {
        let avail = r.fill_buf()?;
        if avail.is_empty() {
            return Err(FrameError::UnexpectedEof);
        }
        let take = (buf.len() - read).min(avail.len());
        buf[read..read + take].copy_from_slice(&avail[..take]);
        r.consume(take);
        read += take;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn write_then_read_roundtrip() {
        let mut out: Vec<u8> = Vec::new();
        write_frame(&mut out, b"{\"a\":1}").unwrap();
        let s = String::from_utf8(out.clone()).unwrap();
        assert!(s.starts_with("Content-Length: 7\r\n\r\n"));
        let mut r = Cursor::new(out);
        let frame = read_frame(&mut r).unwrap().unwrap();
        assert_eq!(frame, b"{\"a\":1}");
    }

    #[test]
    fn multiple_frames_back_to_back() {
        let mut out: Vec<u8> = Vec::new();
        write_frame(&mut out, b"{\"n\":1}").unwrap();
        write_frame(&mut out, b"{\"n\":22}").unwrap();
        let mut r = Cursor::new(out);
        let f1 = read_frame(&mut r).unwrap().unwrap();
        let f2 = read_frame(&mut r).unwrap().unwrap();
        let f3 = read_frame(&mut r).unwrap();
        assert_eq!(f1, b"{\"n\":1}");
        assert_eq!(f2, b"{\"n\":22}");
        assert!(f3.is_none());
    }

    #[test]
    fn missing_content_length_errors() {
        let mut r = Cursor::new(b"X-Weird: 1\r\n\r\n{}".to_vec());
        let err = read_frame(&mut r);
        assert!(matches!(err, Err(FrameError::MissingContentLength)));
    }

    #[test]
    fn invalid_content_length_errors() {
        let mut r = Cursor::new(b"Content-Length: abc\r\n\r\n".to_vec());
        let err = read_frame(&mut r);
        assert!(matches!(err, Err(FrameError::InvalidContentLength(_))));
    }

    #[test]
    fn short_body_errors() {
        let mut r = Cursor::new(b"Content-Length: 10\r\n\r\nabc".to_vec());
        let err = read_frame(&mut r);
        assert!(matches!(err, Err(FrameError::UnexpectedEof)));
    }

    #[test]
    fn eof_before_frame_returns_none() {
        let mut r = Cursor::new(Vec::<u8>::new());
        let f = read_frame(&mut r).unwrap();
        assert!(f.is_none());
    }

    #[test]
    fn unknown_headers_are_ignored() {
        let mut out: Vec<u8> = Vec::new();
        out.extend_from_slice(b"X-Custom: hi\r\n");
        out.extend_from_slice(b"Content-Length: 2\r\n");
        out.extend_from_slice(b"\r\n");
        out.extend_from_slice(b"{}");
        let mut r = Cursor::new(out);
        let f = read_frame(&mut r).unwrap().unwrap();
        assert_eq!(f, b"{}");
    }
}
