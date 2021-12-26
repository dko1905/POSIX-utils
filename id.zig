const std = @import("std");

const ArrayList = std.ArrayList;
const unix = std.os.linux;
const gid_t = unix.gid_t;
const uid_t = unix.uid_t;
const Allocator = std.mem.Allocator;
const Gpa = std.heap.GeneralPurposeAllocator(.{});
const unistd = @cImport({
    @cInclude("unistd.h");
    @cInclude("errno.h");
    @cInclude("grp.h");
    @cInclude("string.h");
});

const Error = error{
    GetGroupsError,
    GetUsernameError,
    GetGroupnameError,
};

const Passwd = extern struct { pw_name: [*:0]const u8, pw_uid: uid_t, pw_gid: gid_t, pw_dir: [*:0]const u8, pw_shell: [*:0]const u8 };

const Group = extern struct { gr_name: [*:0]const u8, gr_gid: gid_t, gr_mem: [*:null]?[*:0]u8 };

var G_flag = false; // Print all different group IDs; can be combined with n,r
var g_flag = false; // Print egid only; can be combined with n,r
var n_flag = false; // Print names instead of numbers; can be combined with g,G,r
var r_flag = false; // Print real id instead of effective; can be combined with n,u,g
var u_flag = false; // Print euid only; can be combined with n,r

pub fn main() !void {
    const out = std.io.getStdOut();
    const w = out.writer();
    var gpa = Gpa{};
    var galloc = gpa.allocator();
    defer _ = gpa.deinit();

    // Retrieve values
    const uid = unix.getuid();
    const uid_name = try getusername(galloc, uid);
    defer galloc.free(uid_name);

    const euid = unix.geteuid();
    const euid_name = try getusername(galloc, euid);
    defer galloc.free(euid_name);

    const gid = unix.getgid();
    const gid_name = try getgroupname(galloc, gid);
    defer galloc.free(gid_name);

    const egid = unix.getegid();
    const egid_name = try getgroupname(galloc, gid);
    defer galloc.free(egid_name);

    var groups = try getgroups(galloc);
    defer galloc.free(groups);

    // Print like GNU id
    std.sort.sort(gid_t, groups, {}, comptime std.sort.desc(gid_t));
    std.sort.sort(gid_t, groups[1..groups.len], {}, comptime std.sort.asc(gid_t));

    const groups_names: [][]u8 = try getgroupnames(galloc, groups);
    defer galloc.free(groups_names);
    defer {
        for (groups_names) |group_name| {
            galloc.free(group_name);
        }
    }

    // Print values
    if (g_flag) {
        if (r_flag) {
            if (n_flag) {
                try w.print("{s}", .{gid_name});
            } else {
                try w.print("{}", .{gid});
            }
        } else {
            if (n_flag) {
                try w.print("{s}", .{egid_name});
            } else {
                try w.print("{}", .{egid});
            }
        }
    } else if (u_flag) {
        if (n_flag) {
            try w.print("{s}", .{euid_name});
        } else {
            try w.print("{}", .{euid});
        }
    } else if (G_flag) {
        if (n_flag) {
            try w.print("{s}", .{gid_name});
            if (std.mem.eql(u8, gid_name, egid_name) != true) {
                try w.print("{s}", .{egid_name});
            }
            for (groups_names) |group| {
                if (std.mem.eql(u8, group, gid_name) or
                    std.mem.eql(u8, group, egid_name))
                {
                    continue;
                }
                try w.print("{s}", .{group});
            }
        } else {
            try w.print("{}", .{gid});
            if (gid != egid) {
                try w.print("{}", .{egid});
            }

            for (groups) |group| {
                if (group == gid or (group == egid)) continue;
                try w.print(" {}", .{group});
            }
            try w.print("\n", .{});
        }
    } else {
        // Default
        try w.print("uid={}({s}) ", .{ uid, uid_name });
        try w.print("gid={}({s})", .{ gid, gid_name });

        if (uid != euid) {
            try w.print(" euid={}({s})", .{ euid, euid_name });
        }
        if (gid != egid) {
            try w.print(" egid={}({s})", .{ egid, egid_name });
        }

        if (groups.len < 1) return;
        try w.print(" groups={}({s})", .{ groups[0], groups_names[0] });
        for (groups[1..groups.len]) |group, i| {
            try w.print(",{}({s})", .{ group, groups_names[i + 1] });
        }

        try w.print("\n", .{});
    }
}

// Reimplement syscall because std is wrong.
fn getgroups(allocator: Allocator) ![]gid_t {
    const cap: c_int = unistd.getgroups(0, null);
    if (cap < 0) {
        return Error.GetGroupsError;
    }

    var arr: []gid_t = try allocator.alloc(gid_t, @intCast(usize, cap));
    if (unistd.getgroups(cap, &arr[0]) < 0) {
        return Error.GetGroupsError;
    }

    return arr;
}

fn getusername(allocator: Allocator, uid: uid_t) ![]u8 {
    const passwd: *Passwd = @ptrCast(*Passwd, unistd.getgrgid(uid));
    const len = @intCast(usize, unistd.strlen(passwd.pw_name));
    var username: []u8 = try allocator.alloc(u8, len);
    errdefer allocator.free(username);

    var i: usize = 0;
    while (i < len) {
        username[i] = passwd.pw_name[i];
        i += 1;
    }

    return username;
}

fn getgroupname(allocator: Allocator, gid: gid_t) ![]u8 {
    const group: *Group = @ptrCast(*Group, unistd.getgrgid(gid));
    const len = @intCast(usize, unistd.strlen(group.gr_name));
    var groupname: []u8 = try allocator.alloc(u8, len);
    errdefer allocator.free(groupname);

    var i: usize = 0;
    while (i < len) {
        groupname[i] = group.gr_name[i];
        i += 1;
    }

    return groupname;
}

fn getgroupnames(allocator: Allocator, groups: []gid_t) ![][]u8 {
    var list: ArrayList([]u8) = ArrayList([]u8).init(allocator);
    errdefer list.deinit();
    try list.ensureTotalCapacity(groups.len);

    for (groups) |group| {
        const groupname: []u8 = try getgroupname(allocator, group);
        list.appendAssumeCapacity(groupname);
    }

    return list.toOwnedSlice();
}
