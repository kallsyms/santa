use std::ffi::CStr;
use futures::stream::StreamExt;
use std::{error::Error, ffi::CString};
use xpc_connection::{Message, MessageError, XpcClient, XpcListener};
use objc;
use objc::{sel, sel_impl};
use objc::runtime::{BOOL, Object};
use std::io::prelude::*;

mod structs;
use structs::*;
mod monarch;

fn validate_client_by_code_signing_requirement(client: &XpcClient) -> bool {
    // TODO
    true
}

async fn handle_client(mut client: XpcClient) {
    println!("New connection");

    if !validate_client_by_code_signing_requirement(&client) {
        return;
    }

    loop {
        match client.next().await {
            None => {
                break;
            }
            Some(Message::Error(MessageError::ConnectionInterrupted)) => {
                println!("The connection was interrupted.");
            }
            Some(m) => {
                let metrics: MetricSet = match m {
                    Message::Dictionary(map) => {
                        let json_msg = map.get(&CString::new("json").unwrap()).expect("Invalid message (no `json`)");
                        match json_msg {
                            Message::Data(json_data) => {
                                serde_json::from_slice(json_data).expect("Decode error")
                            },
                            _ => panic!("Invalid root.json: {:?}", json_msg),
                        }
                    },
                    _ => panic!("Invalid XPC root message: {:?}", m),
                };
                println!("metrics: {:?}", metrics);

                let export_metrics: BOOL;
                let metrics_format: i8; // SNTMetricFormatType
                let metrics_url: String;

                unsafe {
                    // todo: release
                    let config_cls = objc::class!(SNTConfigurator);
                    let configurator: *mut Object = objc::msg_send![config_cls, configurator];

                    export_metrics = objc::msg_send![configurator, exportMetrics];

                    metrics_format= objc::msg_send![configurator, metricFormat];

                    let url: *const Object = objc::msg_send![configurator, metricURL];
                    let string: *const Object = objc::msg_send![url, absoluteString];
                    let ptr: *const std::os::raw::c_char = objc::msg_send![string, UTF8String];
                    let cstr = CStr::from_ptr(ptr);
                    metrics_url = String::from_utf8_lossy(cstr.to_bytes()).to_string();
                }

                if !export_metrics {
                    println!("received metrics message while not configured to export metrics");
                    break;
                }

                let formatted_metrics = match metrics_format {
                    // rawjson
                    1 => {
                        serde_json::to_string(&metrics).expect("Unable to serialize metrics")
                    },
                    // monarchjson
                    2 => {
                        serde_json::to_string(&monarch::convert(&metrics)).expect("Unable to serialize metrics")
                    },
                    _ => panic!("Unsupported metrics format {}", metrics_format),
                };
                println!("formatted: {}", formatted_metrics);

                let metrics_url = url::Url::parse(&metrics_url).expect("Failed to parse URL");
                match metrics_url.scheme() {
                    "file" => {
                        println!("writing to {}", metrics_url.path());
                        let mut file = std::fs::OpenOptions::new()
                            .write(true)
                            .create(true)  // XXX: objc also sets O_TRUNC? https://github.com/google/santa/blob/main/Source/santametricservice/Writers/SNTMetricFileWriter.m#L29
                            .append(true)
                            .open(metrics_url.path())
                            .unwrap();
                        writeln!(file, "{}", formatted_metrics).unwrap();
                    },
                    "http" => {
                    },
                    _ => panic!("Unsupported URL scheme {}", metrics_url.scheme()),
                }
            }
        }
    }

    println!("The connection was invalidated.");
}

#[no_mangle]  // TODO(nickmg): any way to not need this?
#[tokio::main(flavor = "multi_thread")]
async fn main() -> Result<(), Box<dyn Error>> {
    let mach_port_name = CString::new("com.google.santa.metricservice")?;

    println!(
        "Waiting for new connections on {:?}",
        mach_port_name.to_string_lossy()
    );

    let mut listener = XpcListener::listen(&mach_port_name);

    while let Some(client) = listener.next().await {
        tokio::spawn(handle_client(client));
    }

    println!("Server is shutting down");

    Ok(())
}