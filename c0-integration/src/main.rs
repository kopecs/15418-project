use std::{
    fs::File,
    io::{self, Read, Write},
    path::Path,
    process::Command,
};

fn main() -> io::Result<()> {
    let files: Vec<_> = std::env::args().skip(1).collect();
    let processed: Vec<_> = files
        .iter()
        .cloned()
        .map(|x| {
            let mut s = x.trim_end_matches(".c1").to_owned();
            s.push_str("-thr.c1");
            s
        })
        .collect();
    let translated: Vec<_> = processed
        .iter()
        .cloned()
        .map(|mut x| {
            x.push_str(".c");
            x
        })
        .collect();
    let finalised: Vec<_> = translated
        .iter()
        .cloned()
        .map(|x| {
            let mut s = x.trim_end_matches(".c").to_owned();
            s.push_str("-thr.c");
            s
        })
        .collect();

    for (i, o) in files.iter().zip(&processed) {
        preprocess(i, o)?;
    }

    let cc0out = Command::new("cc0")
        .arg("-s")
        .args(&processed)
        .output()
        .expect("failed to run cc0");

    io::stdout().write_all(&cc0out.stdout).unwrap();
    io::stderr().write_all(&cc0out.stderr).unwrap();

    for (i, o) in translated.iter().zip(&finalised) {
        postprocess(i, o)?;
    }

    let gccout = Command::new("gcc")
        .arg("-std=c99")
        .arg("-fwrapv")
        .arg("-Wall")
        .arg("-Wextra")
        .args(&["-o", "a.out"])
        .arg(&format!("-I{}/include", C0_PREFIX))
        .arg(&format!("-I{}/runtime", C0_PREFIX))
        .arg(&format!("-I{}/runtime_test.c1.c", C0_PREFIX))
        .arg(&format!("{}/lib/cc0main.c", C0_PREFIX))
        .arg(&format!("-L{}/runtime", C0_PREFIX))
        .args(&["-Wl,-rpath", &format!("{}/lib", C0_PREFIX)])
        .arg(&format!("{}/lib/libconio.so", C0_PREFIX))
        .arg(&format!("{}/lib/libparse.so", C0_PREFIX))
        .arg(&format!("{}/lib/libstring.so", C0_PREFIX))
        .arg(&format!("{}/lib/libfile.so", C0_PREFIX))
        .args(&["-Wl,-rpath", &format!("{}/runtime", C0_PREFIX)])
        .arg(&format!("{}/runtime/libc0rt.so", C0_PREFIX))
        .arg("thr.so")
        .arg("-g")
        .args(finalised)
        .output()
        .expect("failed to run gcc");

    io::stdout().write_all(&gccout.stdout).unwrap();
    io::stderr().write_all(&gccout.stderr).unwrap();

    Ok(())
}

const STARTPOST: &'static str = "int _c0v___START";
const STARTPRE: &'static str = "int __START";
const SEPARATOR: &'static str = "__";
const END: &'static str = "END = 0;";

const JOINFN: &'static str = "thr_wait";
const FORKFN: &'static str = "thr_add";
const PRELUDE: &'static str = "thr_init();\nthr_start();\n";
const EPILOGUE: &'static str = "thr_finish();\n";

const C0_MAIN: &'static str = "int _c0_main() {";

const C0_PREFIX: &'static str = "/usr/lib/c0";

fn preprocess(i: impl AsRef<Path>, o: impl AsRef<Path>) -> io::Result<()> {
    let mut file = File::open(i)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;

    while let Some(start) = contents.find("do") {
        let end = contents[start..].find(';').expect("invalid") + start;

        let mut record = STARTPRE.to_owned();

        let tokens: Vec<_> = contents[start..end].split_whitespace().collect();
        for line in tokens.chunks(4) {
            let (_, var, _, funapp) = (line[0], line[1], line[2], line[3]);
            record.push_str(var);
            record.push_str(SEPARATOR);

            let funapp: Vec<_> = funapp
                .trim_end_matches(&[')', ';', '\n', '\r', ' '] as &[_])
                .splitn(2, "(")
                .collect();
            record.push_str(funapp[0]);
            record.push_str(SEPARATOR);
            record.push_str(funapp[1]);

            record.push_str(SEPARATOR);
        }

        record.push_str(END);

        contents.replace_range(start..=end, &record[..]);
    }

    let mut outfile = File::create(o)?;
    outfile.write(contents.as_bytes())?;

    Ok(())
}

fn postprocess(i: impl AsRef<Path>, o: impl AsRef<Path>) -> io::Result<()> {
    let mut file = File::open(i)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;

    while let Some(start) = contents.find(STARTPOST) {
        let end = contents[start..].find(END).expect("invalid") + start;

        let mut vars = String::new();
        let mut forks = String::new();
        let mut joins = String::new();
        let mut id = 0;

        let tokens: Vec<_> = contents[start + STARTPOST.len()..end]
            .split(SEPARATOR)
            .filter(|x| !x.is_empty())
            .collect();

        for line in tokens.chunks(3) {
            let (var, func, arg) = (line[0], line[1], line[2]);

            vars.push_str(&format!("void *_c0v_{};\n", var));

            forks.push_str(&format!(
                "int __THRID{} = {}((void * (*)(void *))_c0_{}, _c0v_{});\n",
                id, FORKFN, func, arg
            ));

            joins.push_str(&format!("{}(__THRID{}, &_c0v_{});\n", JOINFN, id, var));

            id += 1;
        }

        contents.replace_range(
            start..=end + END.len(),
            &format!(
                "{}

                {{
                    {}
                    {}
                }}",
                vars, forks, joins,
            ),
        );
    }

    if let Some(start) = contents.find(C0_MAIN) {
        contents.replace_range(start + C0_MAIN.len()..start + C0_MAIN.len(), PRELUDE);
    }

    let mut outfile = File::create(o)?;
    outfile.write(b"#include \"thr.h\"\n")?;
    outfile.write(contents.as_bytes())?;

    Ok(())
}
