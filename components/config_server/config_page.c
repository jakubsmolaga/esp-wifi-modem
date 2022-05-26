#include "config_page.h"

const char *styles = "h1,h2,h3,h4,h5,h6,html{font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,\"Noto Sans\",sans-serif}html{font-size:62.5%}body{font-size:1.8rem;line-height:1.618;max-width:38em;margin:auto;color:#4a4a4a;padding:13px}@media (max-width:684px){body{font-size:1.53rem}}@media (max-width:382px){body{font-size:1.35rem}}h1,h2,h3,h4,h5,h6{line-height:1.1;font-weight:700;margin-top:3rem;margin-bottom:1.5rem;overflow-wrap:break-word;word-wrap:break-word;-ms-word-break:break-all;word-break:break-word}h1{font-size:2.35em}h2{font-size:2em}h3{font-size:1.75em}h4{font-size:1.5em}h5{font-size:1.25em}h6{font-size:1em}p{margin-top:0;margin-bottom:2.5rem}a{text-decoration:none;color:#1d7484}a:hover{color:#982c61;border-bottom:2px solid #4a4a4a}a:visited{color:#144f5a}input[type=button],input[type=reset],input[type=submit]{display:inline-block;padding:5px 10px;text-align:center;text-decoration:none;white-space:nowrap;background-color:#1d7484;color:#f9f9f9;border-radius:1px;border:1px solid #1d7484;cursor:pointer;box-sizing:border-box}input[type=button][disabled],input[type=reset][disabled],input[type=submit][disabled]{cursor:default;opacity:.5}input[type=button]:focus:enabled,input[type=button]:hover:enabled,input[type=reset]:focus:enabled,input[type=reset]:hover:enabled,input[type=submit]:focus:enabled,input[type=submit]:hover:enabled{background-color:#982c61;border-color:#982c61;color:#f9f9f9;outline:0}input,select{color:#4a4a4a;padding:6px 10px;margin-bottom:10px;background-color:#f1f1f1;border:1px solid #f1f1f1;border-radius:4px;box-shadow:none;box-sizing:border-box}input:focus,select:focus{border:1px solid #1d7484;outline:0}input[type=checkbox]:focus{outline:1px dotted #1d7484}label{display:block;margin-bottom:.5rem;font-weight:600}body{display:flex;align-items:center;justify-content:center;background-color:orange}form{text-align:center;background-color:#fff;padding:5%;border-radius:5%}body{display:flex;align-items:center;justify-content:center;background-color:orange}form,div{text-align:center;background-color:#fff;padding:5%;border-radius:5%}";

void send_ssid_option(httpd_req_t *req, wifi_ap_record_t ap)
{
    httpd_resp_sendstr_chunk(req, "<option value=\"");
    httpd_resp_sendstr_chunk(req, (char *)ap.ssid);
    httpd_resp_sendstr_chunk(req, "\">");
    httpd_resp_sendstr_chunk(req, (char *)ap.ssid);
    httpd_resp_sendstr_chunk(req, "</option>");
}

void send_ssid_selector(httpd_req_t *req, wifi_ap_record_t *ap_list, uint16_t ap_count)
{
    httpd_resp_sendstr_chunk(req, "<label for=\"ssid\">SSID:</label>");
    httpd_resp_sendstr_chunk(req, "<select name=\"ssid\">");
    for (uint16_t i = 0; i < ap_count; i++)
        send_ssid_option(req, ap_list[i]);
    httpd_resp_sendstr_chunk(req, "</select>");
}

void send_form(httpd_req_t *req, wifi_ap_record_t *ap_list, uint16_t ap_count)
{
    httpd_resp_sendstr_chunk(req, "<form action=\"/connect\" method=\"post\">");
    httpd_resp_sendstr_chunk(req, "<h1>Water Solution</h1>");
    httpd_resp_sendstr_chunk(req, "<p>Connect to a wifi network</p>");
    send_ssid_selector(req, ap_list, ap_count);
    httpd_resp_sendstr_chunk(req, "<br />");
    httpd_resp_sendstr_chunk(req, "<label for=\"password\">Password:</label>");
    httpd_resp_sendstr_chunk(req, "<input type=\"password\" name=\"password\" />");
    httpd_resp_sendstr_chunk(req, "<br />");
    httpd_resp_sendstr_chunk(req, "<input type=\"submit\" value=\"Connect\" />");
    httpd_resp_sendstr_chunk(req, "</form>");
}

void send_head(httpd_req_t *req)
{
    httpd_resp_sendstr_chunk(req, "<head><title>Connect to a network</title><style>");
    httpd_resp_sendstr_chunk(req, styles);
    httpd_resp_sendstr_chunk(req, "</style></head>");
}

void send_config_page(httpd_req_t *req, wifi_ap_record_t *ap_list, uint16_t ap_count)
{
    httpd_resp_sendstr_chunk(req, "<html>");
    send_head(req);
    httpd_resp_sendstr_chunk(req, "<body>");
    send_form(req, ap_list, ap_count);
    httpd_resp_sendstr_chunk(req, "</body>");
    httpd_resp_sendstr_chunk(req, "</html>");
    httpd_resp_sendstr_chunk(req, NULL);
}

void send_connecting_page(httpd_req_t *req)
{
    httpd_resp_sendstr_chunk(req, "<html>");
    send_head(req);
    httpd_resp_sendstr_chunk(req, "<body>");
    httpd_resp_sendstr_chunk(req, "<div>");
    httpd_resp_sendstr_chunk(req, "<h1>Connecting to the network</h1>");
    httpd_resp_sendstr_chunk(req, "<p>This access point will now disapear and the device will be connected to your network</p>");
    httpd_resp_sendstr_chunk(req, "</div>");
    httpd_resp_sendstr_chunk(req, "</body>");
    httpd_resp_sendstr_chunk(req, "</html>");
    httpd_resp_sendstr_chunk(req, NULL);
}