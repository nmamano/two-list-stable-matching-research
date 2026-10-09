"""Generate a named family in the keyword/order format; standard library only."""
import argparse
from pathlib import Path
import families
import families2
from forcing_weave import change


def format_instance(I):
    lines = ['n ' + str(I['n'])]
    for name, key in zip(('men_x_ranking', 'men_y_ranking', 'women_x_ranking', 'women_y_ranking'), 'abcd'):
        order = sorted(range(I['n']), key=I[key].__getitem__)
        lines.append(name + ' ' + ' '.join(map(str, order)))
    for name, key in (('men_preferences', 'tm'), ('women_preferences', 'tw')):
        lines.append(name + ' ' + ''.join('XY'[t] for t in I[key]))
    return '\n'.join(lines) + '\n'


def generate(kind, k):
    if kind in ('twin', 'weave'):
        if k < 4 or k % 2: raise ValueError('twin parameter k must be even and >=4; n=2k')
        I = families.twin(k)[0]
        return I if kind == 'twin' else change(I, 'boundary', I['n']//16, 'abcd', 91283)
    if kind in ('ltwin', 'ltwin3'):
        if k < (5 if kind == 'ltwin3' else 3) or k % 2 != 1: raise ValueError('parameter must be odd and >=3 (>=5 for ltwin3)')
        return getattr(families2, kind)(k, rev=1)[0]
    if kind == 'ring2':
        if k < 3 or k % 2 != 1: raise ValueError('ring2 parameter must be odd and >=3')
        return families.ring2(k, (k-1)//2)[0]
    if kind == 'gtwin':
        if k < 13 or k % 2 != 1: raise ValueError('gtwin parameter must be odd and >=13')
        return families2.gtwin(k, k, k//2, k//2, k//2)[0]
    raise ValueError(kind)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('family', choices=('twin', 'ltwin', 'ltwin3', 'gtwin', 'weave', 'ring2'))
    p.add_argument('k', type=int)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    try: args.output.write_text(format_instance(generate(args.family, args.k)))
    except ValueError as e: p.error(str(e))
